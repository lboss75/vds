@echo off
set PATH=C:\Program Files\OpenSSL\bin;%PATH%
openssl s_client -showcerts -connect localhost:8000
rem openssl x509 -inform pem -noout -text