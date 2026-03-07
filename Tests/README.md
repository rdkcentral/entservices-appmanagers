As part of rdkservices open source activity and logical grouping of services into various entservices-* repos, the below listed change to L1 and L2 Test are effective hence forth.

# Changes Done:
Since the mock part is common across various plugins/repos and common for L1, L2 & etc, the gtest and gmock related stubs (including platform interface mocks) are maintained in this repository under `Tests/mocks`, and L1/L2 tests remain under the corresponding `Tests` tree.
Hence, any modifications/additions related to mocks and test cases should be committed in this repository under the relevant `Tests` directories.

# Individual Repo Handling
Each individual entservices-* repo was added with a .yml file to trigger L1, L2, L2-OOP test job in github workflow. This yml file triggers below mentioned build jobs in addition to regular build jobs (thunder, thunder tools & etc,).
```
a/ Build mocks => To create TestMock Lib from all required mock relates stubs and copy to install/usr/lib path.
b/ Build entservices-<repo-name> => To create Test Lib of .so type from all applicable test files which are enabled for plugin test.
c/ Build entservices-appmanagers => To create L1/L2  executable by linking the plugins/test .so files.
```
This ensures everything in-tact in repo level across multiple related plugins when there is a new change comes in.

##### Steps to run L1, L2, L2-OOP test locally #####
```
1. checkout the entservices-<repo-name> to your working directory in your build machine.
example: git clone https://github.com/rdkcentral/entservices-deviceanddisplay.git

2. switch to entservices-<repo-name> directory
example: cd entservices-deviceanddisplay

3. check and ensure current working branch points to develop
example: git branch

4. Run below curl command to download act executable to your repo.
example: curl -SL https://raw.githubusercontent.com/nektos/act/master/install.sh | bash

5. Run L1, L2, L2-oop test
example: ./bin/act -W .github/workflows/tests-trigger.yml -s GITHUB_TOKEN=<your access token>

NOTE: By default test-trigger.yml will trigger all tests(L1, L2 and etc) parallely, if you want any one test alone to be triggered/verified then remove the other trigger rules from the tests-trigger.yml
```
# Shared Test Handling
The workflow files in this repository trigger L1, L2, and L2-oop tests for this codebase.

NOTE:
If you face any secret token related error while run your yml, pls comment the below mentioned line
#token: ${{ secrets.RDKE_GITHUB_TOKEN }}

# Execution usecases where manual change required before triggering the test:
```
a/ changes in shared test assets only:
Need to change the `ref` pointer of the checkout job for `entservices-appmanagers` to point to your current working branch.
example:
ref: topic/method_1  /* Checkout entservices-appmanagers job */
uses: rdkcentral/entservices-deviceanddisplay/.github/workflows/L1-tests.yml@topic/method_1

b/ changes in both this repo and individual repo:
Changes mentioned in step (a) above + set the checkout job `ref` for the individual repo branch.
example:
ref: topic/method_1 /* Checkout entservices-appmanagers job */
ref: topic/method_1 /* Checkout entservices-deviceanddisplay-testframework job */
uses: rdkcentral/entservices-deviceanddisplay/.github/workflows/L1-tests.yml@topic/method_1

c/ changes in individual entservices-* repo only
no changes required
```
