build\tests\test_vds_core\Debug\test_vds_core.exe
IF  %ERRORLEVEL% NEQ 0 goto ERROR

build\tests\test_vds_crypto\Debug\test_vds_crypto.exe 
IF  %ERRORLEVEL% NEQ 0 goto ERROR

build\tests\test_vds_data\Debug\test_vds_data.exe 
IF  %ERRORLEVEL% NEQ 0 goto ERROR

build\tests\test_vds_database\Debug\test_vds_database.exe
IF  %ERRORLEVEL% NEQ 0 goto ERROR

build\tests\test_vds_http\Debug\test_vds_http.exe 
IF  %ERRORLEVEL% NEQ 0 goto ERROR

build\tests\test_vds_network\Debug\test_vds_network.exe 
IF  %ERRORLEVEL% NEQ 0 goto ERROR

build\tests\test_vds_parser\Debug\test_vds_parser.exe 
IF  %ERRORLEVEL% NEQ 0 goto ERROR

build\tests\test_vds_scenarios\Debug\test_vds_scenarios.exe 
IF  %ERRORLEVEL% NEQ 0 goto ERROR

:END
exit 0

:ERROR
echo Test failed
exit /b 1
