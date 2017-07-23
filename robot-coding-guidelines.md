Note:  Keywords of any test automation which involves complexity  ( or > 10 lines) should be define in python.
Python program template is available at
https://github.com/openbmc/openbmc-test-automation/blob/master/templates/pgm_template.py and
python coding guidelines available at https://www.python.org/dev/peps/pep-0257/.

# Good Automation code: Expectation:
```
•Automation Code  should be Self Explanatory to all
  –Developer/Tester/End User
•Should be Focused
•Automation code needs to be a specification, not a script
•Should needs to be in domain language
•Should talks about business functionality, not about software design
•Adaptability to the new requirements or new implementation
•Easy to fix the automation code issues
```


# Why RobotFramework?

Predefiend framework and easy to automate test cases using Robot. In case of bug, Much easy to debug when
compare to any other languages.  Good test execution report with details logs and screen shot in case of
GUI automation.

# Robot Coding Guidelines:

## Robot Coding Standard for Test Suite:
```
–Name should be less than 20 characters
–File type should be .robot
  •File name format: eg: <test_inventory.robot>
–Easily readable and self-explanatory
–Suite Setup, Suite Teardown, Test Setup, Test Teardown with proper keywords
–Maximum test suite consists of 50 Test Cases
–Documentation:
  •The documentation should be in the form of an English language command with proper grammar, capitalization and punctuation.
  •E.g. : For a test or keyword named “Get Boot Progress”
  •Correct documentation examples:
    [Documentation] Get boot progress.
    [Documentation] Get the boot progress from the BMC.

  •Incorrect documentation examples:
    [Documentation] This keyword gets the boot progress.
    The prior example is incorrect because it is not in the form of a command
    (i.e. “Do this” or “Do that”).
    [Documentation] Get Boot Progress
    The prior example is incorrect because it is not using good English grammar in that
      a) it is capitalizing words needlessly
      b) it does not end the sentence with a period.
    Additional details should be documented with “#” lines.  Examples:
    # NOTE: A post_test_case call point failure is NOT counted as a boot.
    # Description of argument(s):
    # arg1 This is an example description of arg1
```

## Robot Coding Standard for Test Cases:
```
–Name should be less than 40 characters
–File type should be .robot
–Each word of test case should be capitalized
  •(e.g. ‘Cleanup Users List’ rather than ‘Cleanup users list’).
–Easily readable and self-explanatory
–Test Setup, Test Teardown with proper keywords
–Every test case should have a tag that is equivalent to its name with underscores instead of spaces.
  Example: Test case “Cleanup Users List” must have…
    [Tags]  Cleanup_Users_List
    Other tags may also be included as needed.
–Tests should be independent
–Many high level keywords are fine instead of repeating steps
–Local temporary variables should have prefix “tmp_”
–Line Length – Try to be max of 79 characters wherever possible
  •Use … for continuing in next line – Agree some places not possible such as long if conditions or long string literals.
  •Helps in review and readability
–Documentation:
  •Follow the same rules as for test suite documentation strings (above).
```
## Robot Coding Standard for Resources:
```
–Name should be less than 20 characters
–File type should be .robot
–All characters should be small
–Easily readable and self-explanatory
–General purpose constants which are used by many .robot files should be maintained in a separate resource file
–Application’s data should be maintained in separate resource file
–Documentation:
  •Follow the same rules as for test suite documentation strings (above).
```

## Robot Coding Standard for Keywords:
```
–Name should be less than 40 characters
–Each word in a keyword should begin with a capital letter.  Separate words within a keyword using spaces
  (rather than underscores).  Also, capitalize all letters in acronyms (e.g. DVT, OBMC, SSH).
–Easily readable and self-explanatory
–Prefixes are useful (Eg. Get – Get a value , Set – Set a value)
–Local temporary variables should have prefix “tmp_”
–Complex logics should be in libraries rather than keywords
  •If number of lines more than 10 then keywords can be define in python
–Keyword example:

  •*** Keywords ***
  •Example 1:  This Is Correct
      # This is correct.
  •Example 2:  this_is_incorrect
    # This keyword name is Incorrect because of the underscores instead of spaces and failure to capitalize each word in the keyword.
  •Example 3:  soisthis
    # This keyword name is Incorrect because of a failure to   separate words with spaces and a failure to capitalize each word in the           keyword.
  •Example 4: BMC Is An Acronym
     # This is correct.
–Documentation:
  •Follow the same rules as for test suite documentation strings (above).
```
## Robot Coding Standard for Variables definition:
```
–Name should be less than 40 characters & meaningful words
–Variables should be comprised entirely of lower case letters with the following exceptions which are entirely uppercase:
  •Environment variables (e.g. PATH)
  •Variables intended to be set by robot -v parameters may be all upper case.
  •Built-in robot variables like TEST_NAME, TEST_STATUS, LOG_LEVEL, etc.
–Easily readable and self-explanatory like natural English
–Local temporary variables should have prefix char “tmp_”
–For GUI Automation
  •XPATH variable should start with “xpath”
  •Menu Selection variable should start with “sel” for selection
–Constant variables should be in Capital Letters
–Global variables should be defined at the top of the script
–Test case level variable should be defined at the top of the test case
–Documentation:
  •Follow the same rules as for test suite documentation strings (above).
–Tips:
  For certain very commonly used kinds of variables, kindly observe these conventions to achieve consistency throughout the code.
  •host names
    oWhen a variable is intended to contain either an IP address or a name (either long or short), please give it a suffix of "_host".       Examples:
      •openbmc_host
      •os_host
      •pdu_host
      •openbmc_serial_host

    oFor host names (long or short, e.g. "beye6" or "beye6.aus.stglabs.ibm.com"), use a suffix of _host_name.  Examples:
      •openbmc_host_name

    oFor short host names (e.g. "beye6"), use a suffix of _host_short_name.  Examples:
      •openbmc_host_short_name

    oFor IP addresses, use a suffix of _ip.  Example:
      •openbmc_ip

  •Files and Directories:
    oFiles: If your variable is to contain only the file's name, use a suffix of _file_name.  Examples:
      •ffdc_file_name = "beye6.170428.120200.ffdc"
    oIf your variable is to contain the path to a file, use a suffix of _file_path.  Bear in mind that a file path can be relative or           absolute so that should not be a consideration in whether to use _file_path.
    Examples:
      •status_file_path = "beye6.170428.120200.status"
      •status_file_path = "subdir/beye6.170428.120200.status"
      •status_file_path = "./beye6.170428.120200.status"
      •status_file_path = "../beye6.170428.120200.status"
      •status_file_path = "/gsa/ausgsa/projects/a/autoipl/status/beye6.170428.120200.status"

    oTo re-iterate, it doesn't matter whether the contents of the variable are a relative or absolute path (as shown in the examples            above).  A file path is simply a value with enough information in it for the program to find the file.

    oIf the variable must contain an absolute path (which should be the rare case), use a suffix _abs_file_path.

    oDirectories:
      •Directory variables should follow the same conventions as file variables.
      •If your variable is to contain only the directory's name, use a suffix of _dir_name.  Examples:
      •ffdc_dir_name = "ffdc"
      •If your variable is to contain the path to a directory, use a suffix of _dir_path.  Bear in mind that a dir path can be relative           or absolute so that should not be a consideration in whether to use _dir_path.  Examples:
      •status_dir_path = "status/"
      •status_dir_path = "subdir/status"
      •status_dir_path = "./status/"
      •status_dir_path = "../status/"
      •status_dir_path = "/gsa/ausgsa/projects/a/autoipl/status/"

    oTo re-iterate, it doesn't matter whether the contents of the variable are a relative or absolute path (as shown in the examples            above).  A dir path is simply a value with enough information in it for the program to find the directory.

  •If the variable must contain an absolute path (which should be the rare case), use a suffix _abs_dir_path.

    oIMPORTANT:  As a programming convention, do pre-processing on all dir_path variables to ensure that they contain a trailing slash.        If we follow that convention religiously, that when changes are made in other parts of the program, the programmer can count on the        value having a trailing slash.  Therefore they can safely do this kind of thing:
  •my_file_path = my_dir_path + my_file_name
```

## General Robot Coding Standard:
```
  •Robot supports delimiting cells with either 2 or more spaces or with a pipe symbol "|"
    oFor code to be added in openbmc-test-automation need to follow spaces rather than the pipe character
    oDelimiting cells should be only 2 spaces

  •Make sure all space delimiters in robot code are the minimum 2 characters.  There may be some exceptions to this rule.
    oExceptions to 2 space delim rule:
  •When you wish to line up resource, library or variable values:
    Example:
      Library            Lib1
      Resource        Resource1
    *** Variables ***
    ${var1}           ${EMPTY}
  •When you wish to line up fields for template:
       # LED Name  LED State
         power     On
         power     Off
  •When defining or calling a robot keyword, robot does not care about spaces, underscores or case.  However, we will follow this convention:
    oSeparate words with spaces.
    oCapitalize the first character of each word.
    oCapitalize all characters in any word that is an acronym (e.g. JSON, BMC, etc).
    Examples:
      •Traditional comments (i.e. using the hashtag style comments)
      oPlease leave one space following the hashtag.
      #wrong
      # Right
  •Please use proper English punction:
    oCapitalize the first word in the sentence.
    oEnd sentences with a period.
```
