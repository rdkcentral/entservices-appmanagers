/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2025 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#include <chrono>

#include "DownloadManagerImplementation.h"
#include "UtilsAppManagerTelemetry.h"

#define DOWNLOADER_DOWNLOAD_ID_START        (2000)

/* Idle re-check interval for the downloader thread's condition-variable wait.
 * This is only a periodic backstop: real wakeups arrive immediately via
 * notify_one (new job queued in Download() or shutdown in Deinitialize()).
 * It is intentionally distinct from the retry-backoff seed (waitTime). */
#define DOWNLOADER_IDLE_WAIT_SECONDS        (60)

namespace WPEFramework {
namespace Plugin {

    RDKAM_DEFINE_TELEMETRY_CLIENT(WPEFramework::Plugin::DownloadManagerTelemetryReporting, "downloadManagerBootstrapTime")

    SERVICE_REGISTRATION(DownloadManagerImplementation, 1, 0);

    DownloadManagerImplementation::DownloadManagerImplementation()
        : mDownloadManagerNotification()
        , mDownloaderRunFlag(true)
        , mDownloadId(DOWNLOADER_DOWNLOAD_ID_START)
        , mDownloadPath("")
        , mCurrentservice(nullptr)
    {
        LOGINFO("DM: ctor DownloadManagerImplementation: %p", this);
        mHttpClient = std::unique_ptr<DownloadManagerHttpClient>(new DownloadManagerHttpClient);
    }

    DownloadManagerImplementation::~DownloadManagerImplementation()
    {
        LOGINFO("DM: dtor DownloadManagerImplementation: %p", this);

        std::list<Exchange::IDownloadManager::INotification*>::iterator itDownloader(mDownloadManagerNotification.begin());
        {
            while (itDownloader != mDownloadManagerNotification.end())
            {
                (*itDownloader)->Release();
                itDownloader++;
            }
        }
        mDownloadManagerNotification.clear();
    }

    Core::hresult DownloadManagerImplementation::Register(Exchange::IDownloadManager::INotification* notification)
    {
        LOGINFO("entry");
        ASSERT(notification != nullptr);

        mAdminLock.Lock();
        ASSERT(std::find(mDownloadManagerNotification.begin(), mDownloadManagerNotification.end(), notification) == mDownloadManagerNotification.end());
        if (std::find(mDownloadManagerNotification.begin(), mDownloadManagerNotification.end(), notification) == mDownloadManagerNotification.end())
        {
            mDownloadManagerNotification.push_back(notification);
            notification->AddRef();
        }

        mAdminLock.Unlock();
        LOGINFO("exit");

        return Core::ERROR_NONE;
    }

    Core::hresult DownloadManagerImplementation::Unregister(Exchange::IDownloadManager::INotification* notification)
    {
        LOGINFO();
        ASSERT(notification != nullptr);
        Core::hresult result = Core::ERROR_NONE;

        /* Remove the notification from the list */
        mAdminLock.Lock();
        auto item = std::find(mDownloadManagerNotification.begin(), mDownloadManagerNotification.end(), notification);
        if (item != mDownloadManagerNotification.end())
        {
            notification->Release();
            mDownloadManagerNotification.erase(item);
        }
        else
        {
            LOGERR("DM: Failed to unregister - notification not found!");
            result = Core::ERROR_GENERAL;
        }
        mAdminLock.Unlock();

        return result;
    }

    Core::hresult DownloadManagerImplementation::Initialize(PluginHost::IShell* service)
    {
        Core::hresult result = Core::ERROR_NONE;
        LOGINFO("entry");

        if (service != nullptr)
        {
            mCurrentservice = service;
            mCurrentservice->AddRef();

            LOGINFO("DM: ConfigLine=%s", service->ConfigLine().c_str());
            DownloadManagerImplementation::Configuration config;
            config.FromString(service->ConfigLine());
            if (true == config.downloadDir.IsSet())
            {
		std::lock_guard<std::mutex> lock(mQueueMutex);
                mDownloadPath = config.downloadDir;
	    }
            LOGINFO("DM: downloadDir=%s", mDownloadPath.c_str());
            if (true == config.downloadId.IsSet())
            {
                std::lock_guard<std::mutex> lock(mQueueMutex);
                mDownloadId = static_cast<uint32_t>(config.downloadId.Value());
            }
            int rc = mkdir(mDownloadPath.c_str(), 0777);
            if (rc != 0 && errno != EEXIST)
            {
                LOGERR("DM: Failed to create Download Path '%s' rc: %d errno=%d", mDownloadPath.c_str(), rc, errno);
                result = Core::ERROR_GENERAL;
            }
            else
            {
                LOGINFO("DM: Download path ready at '%s'", mDownloadPath.c_str());
                mDownloadThreadPtr = std::unique_ptr<std::thread>(new std::thread(&DownloadManagerImplementation::downloaderRoutine, this, 1));
            }

            RDKAM_TELEMETRY_INIT(service);
        }
        else
        {
            LOGERR("DM: Initialization failed - service is null!");
            result = Core::ERROR_GENERAL;
        }

        LOGINFO("exit");
        return result;
    }

    Core::hresult DownloadManagerImplementation::Deinitialize(PluginHost::IShell* service)
    {
        Core::hresult result = Core::ERROR_NONE;
        LOGINFO();

        /*  Added lock to avoid race condition with downloader
            Stop the downloader thread */
        {
            std::lock_guard<std::mutex> lock(mQueueMutex);
            mDownloaderRunFlag.store(false, std::memory_order_release);
        }
        mDownloadThreadCV.notify_one();
        if (mDownloadThreadPtr && mDownloadThreadPtr->joinable())
        {
            mDownloadThreadPtr->join();
            mDownloadThreadPtr.reset();
        }

        /* Clear download queues */
        {
            std::lock_guard<std::mutex> lock(mQueueMutex);
            /* Clear priority queue */
            while (!mPriorityDownloadQueue.empty())
            {
                mPriorityDownloadQueue.pop();
            }

            /* Clear regular queue */
            while (!mRegularDownloadQueue.empty())
            {
                mRegularDownloadQueue.pop();
            }
        }

        mCurrentservice->Release();
        mCurrentservice = nullptr;

        DownloadManagerTelemetryReporting::getInstance().reset();

        return result;
    }

    // IDownloadManager methods
    Core::hresult DownloadManagerImplementation::Download(const string& url,
        const Exchange::IDownloadManager::Options &options,
        string &downloadId)
    {
        Core::hresult result = Core::ERROR_GENERAL;

        mAdminLock.Lock();
        if (!mCurrentservice->SubSystems()->IsActive(PluginHost::ISubSystem::INTERNET))
        {
            LOGERR("DM: Download failed - no internet! url=%s priority=%d retries=%u rateLimit=%u",
                   url.c_str(), options.priority, options.retries, options.rateLimit);
            result = Core::ERROR_UNAVAILABLE;
            DownloadManagerTelemetryReporting::getInstance().recordDownloadErrorTelemetry("NO_INTERNET", static_cast<int>(DownloadReason::DOWNLOAD_FAILURE));
        }
        else if (url.empty())
        {
            LOGERR("DM: Download failed - empty URL! priority=%d retries=%u rateLimit=%u",
                   options.priority, options.retries, options.rateLimit);
            DownloadManagerTelemetryReporting::getInstance().recordDownloadErrorTelemetry("EMPTY_URL", static_cast<int>(DownloadReason::DOWNLOAD_FAILURE));
        }
        else
        {
            std::lock_guard<std::mutex> lock(mQueueMutex);
            std::string downloadIdStr = std::to_string(++mDownloadId);
            DownloadInfoPtr newDownload = std::make_shared<DownloadInfo>(url, downloadIdStr, options.priority, options.retries, options.rateLimit);
            if (newDownload != nullptr)
            {
                std::string filename = mDownloadPath + "package" + newDownload->getId();
                newDownload->setFileLocator(filename);

                /* Check the priority download request */
                if (options.priority)
                {
                    /* Added to priority queue download */
                    mPriorityDownloadQueue.push(newDownload);
                }
                else
                {
                    /* Added to regular queue download */
                    mRegularDownloadQueue.push(newDownload);
                }
                LOGINFO("DM: Queued download request: id=%s priority=%d priorityDepth=%zu regularDepth=%zu",
                        newDownload->getId().c_str(), newDownload->getPriority(),
                        mPriorityDownloadQueue.size(), mRegularDownloadQueue.size());
                mDownloadThreadCV.notify_one();
                LOGINFO("DM: Download Request: id=%s url=%s priority=%d retries=%u rateLimit=%u",
                        newDownload->getId().c_str(), newDownload->getUrl().c_str(),
                        newDownload->getPriority(), newDownload->getRetries(),
                        newDownload->getRateLimit());

                /* Returning queued download id */
                downloadId = newDownload->getId();
                result = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("DM: Failed to create new download - url=%s priority=%d retries=%u rateLimit=%u",
                       url.c_str(), options.priority, options.retries, options.rateLimit);
            }
        }
        mAdminLock.Unlock();

        return result;
    }

    Core::hresult DownloadManagerImplementation::Pause(const string &downloadId)
    {
        Core::hresult result = Core::ERROR_GENERAL;

        std::lock_guard<std::mutex> lock(mQueueMutex);
        if (!downloadId.empty() && (mCurrentDownload.get() != nullptr) && mHttpClient)
        {
            if (downloadId.compare(mCurrentDownload->getId()) == 0)
            {
                mHttpClient->pause();
                LOGINFO("DM: downloadId %s paused", downloadId.c_str());
                result = Core::ERROR_NONE;
            }
            else
            {
                LOGWARN("DM: Pause failed - ID mismatch! Requested=%s, Active=%s",
                        downloadId.c_str(), mCurrentDownload->getId().c_str());
                result = Core::ERROR_UNKNOWN_KEY;
            }
        }
        else
        {
            LOGERR("DM: Pause failed - downloadId=%s mCurrentDownload=%p mHttpClient=%p",
                   downloadId.c_str(), mCurrentDownload.get(), mHttpClient.get());
        }

        return result;
    }

    Core::hresult DownloadManagerImplementation::Resume(const string &downloadId)
    {
        Core::hresult result = Core::ERROR_GENERAL;

        std::lock_guard<std::mutex> lock(mQueueMutex);
        if (!downloadId.empty() && (mCurrentDownload.get() != nullptr) && mHttpClient)
        {
            if (downloadId.compare(mCurrentDownload->getId()) == 0)
            {
                mHttpClient->resume();
                LOGINFO("DM: downloadId %s resumed", downloadId.c_str());
                result = Core::ERROR_NONE;
            }
            else
            {
                LOGWARN("DM: Resume failed - ID mismatch! Requested=%s, Active=%s",
                        downloadId.c_str(), mCurrentDownload->getId().c_str());
                result = Core::ERROR_UNKNOWN_KEY;
            }
        }
        else
        {
            LOGERR("DM: Resume failed - downloadId=%s mCurrentDownload=%p mHttpClient=%p",
                   downloadId.c_str(), mCurrentDownload.get(), mHttpClient.get());
        }

        return result;
    }

    Core::hresult DownloadManagerImplementation::Cancel(const string &downloadId)
    {
        Core::hresult result = Core::ERROR_GENERAL;

        std::lock_guard<std::mutex> lock(mQueueMutex);
        if (!downloadId.empty() && (mCurrentDownload.get() != nullptr) && mHttpClient)
        {
            if (downloadId.compare(mCurrentDownload->getId()) == 0)
            {
                mCurrentDownload->cancel();
                mHttpClient->cancel();
                LOGINFO("DM: downloadId %s cancelled", downloadId.c_str());
                result = Core::ERROR_NONE;
            }
            else
            {
                LOGWARN("DM: Cancel failed - ID mismatch! Requested=%s, Active=%s",
                        downloadId.c_str(), mCurrentDownload->getId().c_str());
                result = Core::ERROR_UNKNOWN_KEY;
            }
        }
        else
        {
            LOGERR("DM: Cancel failed - downloadId=%s mCurrentDownload=%p mHttpClient=%p",
                   downloadId.c_str(), mCurrentDownload.get(), mHttpClient.get());
        }

        return result;
    }

    Core::hresult DownloadManagerImplementation::Delete(const string &fileLocator)
    {
        if (fileLocator.empty())
        {
            LOGERR("DM: Delete failed - fileLocator is empty!");
            return Core::ERROR_BAD_REQUEST;
        }

        Core::hresult result = Core::ERROR_GENERAL;

        {
            std::lock_guard<std::mutex> lock(mQueueMutex);
            if (!fileLocator.empty() && (mCurrentDownload.get() != nullptr) && \
                (fileLocator.compare(mCurrentDownload->getFileLocator()) == 0))
            {
                LOGWARN("DM: fileLocator %s download is in-progress", fileLocator.c_str());
                return result;
            }
        }

        if (remove(fileLocator.c_str()) == 0)
        {
            LOGINFO("DM: fileLocator %s Deleted", fileLocator.c_str());
            result = Core::ERROR_NONE;
        }
        else
        {
            LOGERR("DM: fileLocator '%s' delete failed", fileLocator.c_str());
        }

        return result;
    }

    Core::hresult DownloadManagerImplementation::Progress(const string &downloadId, uint8_t &percent)
    {
        Core::hresult result = Core::ERROR_GENERAL;

        std::lock_guard<std::mutex> lock(mQueueMutex);
        if (!downloadId.empty() && (mCurrentDownload.get() != nullptr) && mHttpClient)
        {
            if (downloadId.compare(mCurrentDownload->getId()) == 0)
            {
                percent = mHttpClient->getProgress();
                LOGINFO("DM: Download Progress percent %u", percent);
                result = Core::ERROR_NONE;
            }
            else
            {
                result = Core::ERROR_UNKNOWN_KEY;
            }
        }
        else
        {
            LOGERR("DM: Progress failed - downloadId=%s mCurrentDownload=%p mHttpClient=%p",
                   downloadId.c_str(), mCurrentDownload.get(), mHttpClient.get());
        }

        return result;
    }

    Core::hresult DownloadManagerImplementation::RateLimit(const string &downloadId, const uint32_t &limit)
    {
        Core::hresult result = Core::ERROR_GENERAL;

        std::lock_guard<std::mutex> lock(mQueueMutex);
        if (!downloadId.empty() && (mCurrentDownload.get() != nullptr) && mHttpClient)
        {
            if (downloadId.compare(mCurrentDownload->getId()) == 0)
            {
                LOGINFO("DM: downloadId='%s' limit=%u", downloadId.c_str(), limit);
                mCurrentDownload->setRateLimit(limit);
                mHttpClient->setRateLimit(limit);
                result = Core::ERROR_NONE;
            }
            else
            {
                LOGWARN("DM: '%s' download is not active - unable to set rate limit=%u!", downloadId.c_str(), limit);
                result = Core::ERROR_UNKNOWN_KEY;
            }
        }
        else
        {
            LOGERR("DM: Set RateLimit Failed - mCurrentDownload=%p", mCurrentDownload.get());
        }

        return result;
    }

    void DownloadManagerImplementation::downloaderRoutine(int waitTime)
    {
        while (mDownloaderRunFlag.load(std::memory_order_acquire))
        {
            DownloadInfoPtr downloadRequest = nullptr;
            {
                std::unique_lock<std::mutex> lock(mQueueMutex);
                // Use a bounded wait to avoid indefinite blocking and to periodically re-check the run flag / queues
                const bool ready = mDownloadThreadCV.wait_for(lock,std::chrono::seconds(DOWNLOADER_IDLE_WAIT_SECONDS), [&] {
                    return !mDownloaderRunFlag.load(std::memory_order_acquire) || !mPriorityDownloadQueue.empty() || !mRegularDownloadQueue.empty();
                });
                if (!mDownloaderRunFlag.load(std::memory_order_acquire))
                    break;
                if (!ready) {
                    continue;
                }
                downloadRequest = pickDownloadJob();
            } //unlock mutex

            if (!downloadRequest)
            {
                LOGWARN("DM: No download request available - continuing loop!");
                continue;
            }

            DownloadManagerHttpClient::Status status = DownloadManagerHttpClient::Status::Success;
            int attemptCount = 0;

            LOGINFO("DM: Starting downloadId=%s url=%s file=%s retries=%d rateLimit=%u",
                    downloadRequest->getId().c_str(), downloadRequest->getUrl().c_str(),
                    downloadRequest->getFileLocator().c_str(), downloadRequest->getRetries(),
                    downloadRequest->getRateLimit());

            time_t downloadStartTime = DownloadManagerTelemetryReporting::getInstance().getCurrentTimestampMs();

            for (int i = 0; i < downloadRequest->getRetries(); ++i)
            {
                attemptCount = i + 1;
                if (i > 0)
                {
                    int retryWaitTime = nextRetryDuration(waitTime);
                    LOGDBG("DM: Retry %d/%d: Waiting %d seconds before retrying...",
                            attemptCount, downloadRequest->getRetries(), retryWaitTime);
                    std::this_thread::sleep_for(std::chrono::seconds(retryWaitTime));

                    if (downloadRequest->cancelled())
                    {
                        LOGINFO("DM: Download cancelled: id=%s !", downloadRequest->getId().c_str());
                        break;
                    }
                }

                LOGDBG("DM: Attempting download (%d/%d): id=%s url=%s file=%s rateLimit=%u",
                        attemptCount, downloadRequest->getRetries(),
                        downloadRequest->getId().c_str(),
                        downloadRequest->getUrl().c_str(),
                        downloadRequest->getFileLocator().c_str(),
                        downloadRequest->getRateLimit());

                auto begin = std::chrono::steady_clock::now();
                status = mHttpClient->downloadFile(downloadRequest->getUrl(),
                                        downloadRequest->getFileLocator(),
                                        downloadRequest->getRateLimit());
                auto end = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                long httpCode = mHttpClient->getStatusCode();
                if (status == DownloadManagerHttpClient::Status::Success)
                {
                    LOGINFO("DM: Download succeeded (took %lldms): id=%s url=%s file=%s retries=%d rateLimit=%u http_code=%ld",
                    elapsed, downloadRequest->getId().c_str(), downloadRequest->getUrl().c_str(),
                    downloadRequest->getFileLocator().c_str(), downloadRequest->getRetries(),
                    downloadRequest->getRateLimit(), httpCode);
                    break;
                }

                if (httpCode == 404)
                {
                    LOGERR("DM: Download file not found (404) - id=%s url=%s status=%d",
                            downloadRequest->getId().c_str(), downloadRequest->getUrl().c_str(), status);
                    status = DownloadManagerHttpClient::Status::HttpError;
                    break;
                }

                LOGDBG("DM: Attempt download (%d/%d): status=%d http_code=%ld elapsed=%lld ms",
                        attemptCount, downloadRequest->getRetries(), status, httpCode, elapsed);
            }

            if (status != DownloadManagerHttpClient::Status::Success)
            {
                LOGERR("DM: Download failed %d/%d: id=%s status=%d",
                       attemptCount, downloadRequest->getRetries(), downloadRequest->getId().c_str(), status);
            }

            DownloadReason reason = static_cast<DownloadReason>(DOWNLOAD_REASON_NONE);
            int64_t totalDownloadTime = DownloadManagerTelemetryReporting::getInstance().getCurrentTimestampMs() - downloadStartTime;
            switch (status)
            {
                case DownloadManagerHttpClient::Status::DiskError:
                    reason = DownloadReason::DISK_PERSISTENCE_FAILURE;
                    LOGERR("DM: Download failed due to disk error: id=%s", downloadRequest->getId().c_str());
                    DownloadManagerTelemetryReporting::getInstance().recordDownloadErrorTelemetry(downloadRequest->getId(), static_cast<int>(DownloadReason::DISK_PERSISTENCE_FAILURE));
                    break;

                case DownloadManagerHttpClient::Status::HttpError:
                    reason = DownloadReason::DOWNLOAD_FAILURE;
                    LOGERR("DM: Download failed due to HTTP error: id=%s", downloadRequest->getId().c_str());
                    DownloadManagerTelemetryReporting::getInstance().recordDownloadErrorTelemetry(downloadRequest->getId(), static_cast<int>(DownloadReason::DOWNLOAD_FAILURE));
                    break;

                default:
                    DownloadManagerTelemetryReporting::getInstance().recordDownloadTimeTelemetry(downloadRequest->getId(), totalDownloadTime);
                    break; /* Do nothing */
            }

            notifyDownloadStatus(downloadRequest->getId(), downloadRequest->getFileLocator(), reason);
            {
                std::lock_guard<std::mutex> lock(mQueueMutex);
                mCurrentDownload.reset();
            }
["fileLocator"] = locator;
        if (reason != (DownloadReason)DOWNLOAD_REASON_NONE)
        {
            obj["failReason"] = getDownloadReason(reason);
        }
        list.Add(obj);
        std::string jsonstr;
        if (!list.ToString(jsonstr))
        {
            LOGERR("DM: Failed to stringify JsonArray");
        }
        else
        {
            mAdminLock.Lock();
            LOGDBG("DM: OnAppDownloadStatus event: '%s'", jsonstr.c_str());
            for (auto notification: mDownloadManagerNotification)
            {
                notification->OnAppDownloadStatus(jsonstr);
            }
            mAdminLock.Unlock();
        }
    }

    DownloadManagerImplementation::DownloadInfoPtr DownloadManagerImplementation::pickDownloadJob(void)
    {
        if ((!mPriorityDownloadQueue.empty() || !mRegularDownloadQueue.empty()) && mCurrentDownload == nullptr)
        {
            if (!mPriorityDownloadQueue.empty())
            {
                /* Priority queue download request */
                mCurrentDownload = mPriorityDownloadQueue.front();
                mPriorityDownloadQueue.pop();
                LOGINFO("DM: Dequeued priority download request: DownloadId=%s urlBytes=%zu file=%s rateLimit=%u remainingPriority=%zu remainingRegular=%zu",
                    mCurrentDownload->getId().c_str(), mCurrentDownload->getUrl().size(),
                        mCurrentDownload->getFileLocator().c_str(), mCurrentDownload->getRateLimit(),
                        mPriorityDownloadQueue.size(), mRegularDownloadQueue.size());
            }
            else
            {
                /* Regular queue download request */
                if (!mRegularDownloadQueue.empty())
                {
                    mCurrentDownload = mRegularDownloadQueue.front();
                    mRegularDownloadQueue.pop();
                        LOGINFO("DM: Dequeued regular download request: DownloadId=%s urlBytes=%zu file=%s rateLimit=%u remainingPriority=%zu remainingRegular=%zu",
                            mCurrentDownload->getId().c_str(), mCurrentDownload->getUrl().size(),
                            mCurrentDownload->getFileLocator().c_str(), mCurrentDownload->getRateLimit(),
                            mPriorityDownloadQueue.size(), mRegularDownloadQueue.size());
                }
            }
        }
        return mCurrentDownload;
    }

} // namespace Plugin
} // namespace WPEFramework
