#include "stdafx.h"
#include "private/cert_control_p.h"
#include "cert_control.h"

char vds::cert_control::root_certificate_[1821] =
"MIIFTzCCAzegAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA5MTUxNzE5MzNaFw0xOTA5MTUxNzE5"
"MzNaMDoxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRkwFwYDVQQDDBB2YWRpbUBpdi1zb2Z0"
"LnJ1MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEArawD8I49frRwoz4r4FfvTG2njHJ87s8w"
"h/dJYCR29jENDbto3nBB4QPMlgJBRbPBFY+xiYumOoFpD1QLZu5BWPPL27r+B+Q+ObkxaAE8CB4ueR4+"
"5gQG0V7VjAPFTlkMUgCOCU/0walTRntIBmVGdXPcf0e36htS/s/kPjnGdimsPGGmlmGWmruIlkGQVusZ"
"tJi9kfp7BQ/cPrW4htHM0upZUSrH44cdPBfWKPnVcnWpm5f+s2wItaMq/UEEYTrmi9diT02zwiufl+cH"
"6y0iLCbpqRhUnfN+UtfphkWFUyip45PUW4y3MHobpJMxT+UrxRFBc6DfXmSOdLU9IIClB4i35UvR4ESY"
"wBydA7fSdZIfDU/G1AKaGDZIKBIv93TmSMtVT6RV02F6ciCXh8H2pAlO764iGjfPBkplpGi1ixYUGrVD"
"/1NFrxDPBUumxQTdIjoQP2cJ2QDU5nT7OE/7mfjX56GTKEsQPeSbkOJpw4g0bQ7MiUb7WcXM/ei+My2b"
"VMvNnsLgFOErDoxWrlLOkSSFtS+wuJlMlIl/S/VLuhlSd2qx3g1uUwm63WGhCvtHLpJ+vb6fYLSOOP3W"
"Hb75v0rCTC3ZOZmCNq5/F1Rk3BiaOuvRnsc79/imEkJpag9J83OM7n/Wm9FvQ646/cRNApYCPO88ujnP"
"MVA1QVtlbRMCAwEAAaNgMF4wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwEQYJYIZIAYb4"
"QgEBBAQDAgIEMCgGCWCGSAGG+EIBDQQbFhlleGFtcGxlIGNvbW1lbnQgZXh0ZW5zaW9uMA0GCSqGSIb3"
"DQEBCwUAA4ICAQBobXI3iCGazf6K8V1RGLLyUYX/ZuuozbfjszavJqjr9SkPG+lX3iLig3SrTW/f13OX"
"aYL28uqZZH3zgA/xeEu1RTAMgt9aw5nEx8CRuYMXrG3vY9nHkGae9ede4BgSa9nPlprWNkwbekkWLEMm"
"baK/DDmbgND5LF7bKilck18Ep50kRp0WocjFYrARu2sb/7C9RyqTPW7RiS/djikSuJz2Dk3XjOUnAJjl"
"96x0SVu3RDnKbA8Q++j1TV8sGcPnsWw1t5iTz9qfoLkEmIy6vj+xVV1dOhF8REN4B4KJBPdlx5LHsGtZ"
"z76lkM7BsMTN1TOuEf/cuvyDQ59Qzv3iUte7V8Hp2t6L3kvX1LfWF1m3O60i5mlSYXuev021xyanoALF"
"J+JArvFqrHHah1KItok9wX8yWjQ6RD4rn9DGHLckg3fiCSx5JcmOx+cjhjz3hX5Xb5ZC6f7YcBmZfkey"
"plzjw6XJwqeZZ38lfYofiJPaUKQg+RPCPnJgiEnbiXcoz4iI5M4QLZHyMXEJwIJ7iVhbzV/8Ijyjjt/0"
"HBfDyN1uL//Xyr7/BPjt9aNM/ZeJYJ02eBJjDJkbgCpHqN2W/5ejDIWhEKt3wUQ74EjMSUYfMgvEc+H9"
"i+fTKK5RSRwk4OM5mknVXJCQDQLmx618qTFkuGGZ3vtXNYVYfj06YJFLpQ==";
char vds::cert_control::common_news_read_certificate_[1821] =
"MIIFTzCCAzegAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA5MTUxNzE5MzRaFw0xOTA5MTUxNzE5"
"MzRaMDoxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRkwFwYDVQQDDBBDb21tb24gTmV3cyBS"
"ZWFkMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA5W3BtF4gD8VjEmZWmeekBFugkkKPg7ML"
"IjDlXBPi1QijwZOdiTcUJ+1tvju8kbolm7DSrWTprBCeV0cnucDYj11gNGOM0SYdeLEFYgabFjeveeN3"
"bkBn7oWs5dFCiapqj6/MI0jhG0xRQ5VIFUZ4rPvLprJoQYm9WmtxKaMvjoUH0quHGJUlXaSIOWIX/DSw"
"SrcopAZoMljD1uOtKr0Z1LLcYbRes8W75pMQONCg/bo2d09GwyCUARG0MBKu6xFuWkMEdqGK1r5//pxm"
"1tOcpZJNVRh2kpIHXRJzYIk8uoAIJcrHCV/tiXFwFardM78D1iGOeQWMnMJejTEeQGYBfFkuYVJE8ZKq"
"OEmw9DlZckrA7w/HkOg3SN96aZMps8+xf/UOFks1c6kMFOz6xsD3cNLauLEmjFKoyOEIjguFDDxSHuS8"
"ShJr6OxShdA995+vu1Lih5dkbi2RYIfMfa8lChCKk9wrSk+gvNRjMzgzLMfoEZHSrFw/WyKgqxeVZ370"
"JbqO5GkuHLLT0OajQLBU9fFVTLsUXJ2qO01wYaBs2BX5p1jUTJagrYj9jsiikkPwr50SbjpmRjwIlH+b"
"qnCcrxS8JXPWnxxV+aURq5eZgOJyngzMh2lL1MPwZ8lRyAEXkqgDNdV3XPmR5g40UonWscjZfnQxjlAV"
"KIsz8pnqd8ECAwEAAaNgMF4wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwEQYJYIZIAYb4"
"QgEBBAQDAgIEMCgGCWCGSAGG+EIBDQQbFhlleGFtcGxlIGNvbW1lbnQgZXh0ZW5zaW9uMA0GCSqGSIb3"
"DQEBCwUAA4ICAQBSNmrXUrtpww2eoUyeJlJmBH32WgZhuBROcpZlVZu84DsLxgQUEo+FGohiA3dS7lQb"
"EteIfy7xGBNPLfAAT6GDidlnvGlsWBP5JZG8ZX9hv9Lse2404JP4APmHEFIq/87hJNef/dvJlB+Po2TO"
"4hi4cqkZsgnteczFxwdIpfeT0CnfZXMhTKWEdvB5OMsjW11LwLaC2WBmpOg4CrTt1IkFovvn8gKjJpLL"
"pymEdR+bshtMjB+bZHFt35kz8Cv/zPxU83QYTWNIDSEKdbfHc++BSGB21Lj+88n8Z2aoySzcCkiZjxkd"
"EGWN+5K3l6G71SWwwsC+PKg9JBNGxIomiSbcSNSqdGuzqGVGs2jQm0E7V5XPGYShNnBqL8ftsXwQwgor"
"70Jur+KVANOAzW5mrxiLVvIvv/FiyZ8gb0003VdT/UCpTRMDTeWfA+vP8ZaX8UOlkHWzRFN8IkPiiqES"
"cPkZpeTLJ4lCiUlW0PGWo0qo6sMMh5W0BKKdxgfdc7Q3vAXHY6svaipnoVb1NSJ8H3zHPNUqSUNUeGLu"
"ZastdtFKZ/Xk03JeYPOlVh1jKh5UEnsaqRKr/AFQLkaGaE3+jvokiSuGM2ebg6Xj8sCanYrrQTU8IjIu"
"HfIrccJ/QQ9YQ0tYpV/b4wf5ojsvld4PFHWXJwAJ/gbSmvy/L3P2E2gjqQ==";
char vds::cert_control::common_news_read_private_key_[3137] =
"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"
"jdHgmmMyNxLI";
char vds::cert_control::common_news_write_certificate_[1821] =
"MIIFUDCCAzigAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA5MTUxNzE5MzRaFw0xOTA5MTUxNzE5"
"MzRaMDsxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRowGAYDVQQDDBFDb21tb24gTmV3cyBX"
"cml0ZTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAKPhlfjP58vZh89XWlKiSdqbUj3wMzXb"
"yhRudP/vvB/21lo0sZGrQ1exJkFY798ruqhmfPfDCoDHBz7bhJPLArgrw8lIuuf7aNWaAcmnfrl6MC0l"
"zXneW2ysvuv9jXq1Kv2RoSeNP6Ar0wUjHW5Nd36qIjmsVaMiHsIFMFxsbIdlaeTxBKrD+WpPEgJzqMcm"
"FfjASnTs1tmC+554IMxmZfbbtQpKQ38mOca7axSUsOKnVJ3yimWjat/fC45LL1j1EAmJNF21Z6gnS/dH"
"9veFjIWipTTcUX3tCdKCOEMle/mjwE2Ud58FIY0MQAeILWBHKAfJbWZvsI5Te/HeHkasCId2Kn1Bmdse"
"IIiACQKVQRmUe3UtH8A6/bb6DJYP8m9dJA05ztA/crqBU29wr9GHoS72nYB0is52s5jJBsK4xNn0Lf/n"
"sGA58bc5FM7qZ7CCva4E3b3egItPy6i7hRCwkworDIrnploDwfXkgJ6T9MBfq8p90mW2GbfA/fvbvnz9"
"20pExgiifBE5pLU9ZED+la2JQHi93PbrMQu6iByY2X3Z/v78Tl76+Yvny2RDb2IX1T08C7pKsCXp/KSE"
"MktFylGP6GzL4480wwSd0KfVGJ1OyUVbRLwZoYRjEbwFReW2CTuFvElMjrd0AkpK9eIkzRiTXbyyulHT"
"SekTgtK3wn8zAgMBAAGjYDBeMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMBEGCWCGSAGG"
"+EIBAQQEAwICBDAoBglghkgBhvhCAQ0EGxYZZXhhbXBsZSBjb21tZW50IGV4dGVuc2lvbjANBgkqhkiG"
"9w0BAQsFAAOCAgEAlblVjPBJuuoYGvc0C5B2iMD6L04tklSDKhh2G4VfMN55c+qQTbL2yrOt8T7ww4uc"
"oB6XlKZpheccJg38FxEYFOpqfAp3bm4Gp2Lhik1ZBAES3Y26JsbNY+Aik9Hy096XZ03ZtESbUafKBMxb"
"757ycJyD5TozLSNdouT0BIG0ZQ881XWgIZiBCCOxJ/kM8nEG3A3+gqfs3zV1o44mZQbAfwcJ8J04P2i3"
"er2XDY4rTUPIs3qP3TC7tWfCyycGU+/7dZB+CY7d4aq0ON8CfQyH+NYeK4H3/1bfqyjj1axHMk15VDNd"
"LpMSrTJV3EGyDhPI82jiu54qI/LizWWlGOdv6LUJGrC4UymWtmcE7k9zc5d5KBNvvRvC2jSeXHX0fMtA"
"wcMmuVIMlHukXdtdwzN/vxf2RDAYiqUtz5f1kbq2QGyxlclXZdmYktxb04+QofmT03MKEY/P/qvWpGpI"
"ZEo+tTW6a1R7c8KkvfgrX1/TkPKGZmpuNeoJJYerI3fbC09jikB38GFoN3gCSJU2cx0UA0qAFIRLP8j8"
"NimYM6q1WV5Kdcu4U2/kqSTr8j94O3y3w60/G415sdt3ocYE2YfRUV+NZfVOWIeTtjVsON7k8UneGf+U"
"gwbFT7W4dS1vmdIaj2JHJUtkeA+DZqWmKVXwgqswkdKazYxJNDSValhYfQY=";
char vds::cert_control::common_news_admin_certificate_[1821] =
"MIIFUDCCAzigAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA5MTUxNzE5MzRaFw0xOTA5MTUxNzE5"
"MzRaMDsxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRowGAYDVQQDDBFDb21tb24gTmV3cyBB"
"ZG1pbjCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAMfhz13ZChih69KHSZ1rG5n+VxGf8L8M"
"CJfkgjjLRl8zO09ssZXY3ajwxgN1w4FEHKWtTESd/2xqqWbch3GlyIXdR+oTdLI3IJ9Wv3s3kl5YtCbe"
"y0J9lAYdlgiYJoPVyatiEiTdvEu2/1+cfPYhXt8VWCNb8D2fgA1+rxCh2P9W1TispKEc2ZsRcpynz37Y"
"JhiQT5LzA/VSZRsnHLzYukmxOM5fP7f7DtZUkiGQehDlP0z7hMXrw8GVfX5vJ3Cc8h+E9Qbkhc7JH4st"
"Rs/dmp8nEPUFnqRoR92025ZEaxgGImpEh6n8v0hhkpgD3F8ACb3G5aAnwMGWrj/twxWOu5bYEx7SKUt5"
"y/rPZNiEOEPO1Shw21eo8effyea/dvrJeTIVBJ3vVuYvqLwE2cZYgagG7yx84WTWtYCoPZL4TWQEiTL2"
"WAkN0U/iVUI0Fgw/TbYqbtbXzFJkWRWGoW3Swe9w5dYSuZ50NGlmYNgKABLJc/Gfr3QRH1E4PfLgzW5Q"
"Gfgb1BIe9JF1EbNt/es5GKmCEKMyM1fMepF8Tg4K0GJzqkZlrgLglk1Rm3Pxi/mEWRtQR5TMN8wJmnQP"
"Va03cjdV0v+3C12te1yCO30DHzbJGhwMlG4z19k5UYzzImsX67XNzLwDiwKi/zArREvmx8iS2qYrwhZt"
"ngfgrb+WM2htAgMBAAGjYDBeMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMBEGCWCGSAGG"
"+EIBAQQEAwICBDAoBglghkgBhvhCAQ0EGxYZZXhhbXBsZSBjb21tZW50IGV4dGVuc2lvbjANBgkqhkiG"
"9w0BAQsFAAOCAgEAhZhEc0rXBsmJNgdsfA5ycRJj/kYpBZVRJTRupxrwrpyZkhLtzWSX1L0PItaWM+IB"
"6FjfWivhMNmaIW1B8FanahLOOkDp6EdhQqw7Idw1d+509HqZStQHBp1wCbPwI9mQgZhivj09ObzNJgCL"
"ZfU35UdF1bu3ED4era3bQm6L69/5CXmnbvbRxH1/Mk4ATfvvpCH5D4yra1IocwY1+t4BNDqvrXSEjBza"
"BCkIg9Y7e/m511osQXgNZDIf590AtWQ4/4TzXL3UYrcm+JgGYutQXt0RkMbpT+xuh1bLIgBoRYiiVZh6"
"nmhJ3ne/kulT39OoOMMH0AeOTKHLy0gvW8m9tUJtthBoTJ4qQvK81gl6ENVfW8VgSSDU3pLl6NPz1Bgy"
"FQJkPiW6B2JNxsz+VejMKCe80rRevn39OCjh3InV67j2dWP6jWPxQpmJ8wSzVg1V4SfpMzhS/pHIf/yi"
"2XNMUoP8HIcWtpmMrc/ZzxTGJsa3bTQSjRFVGOWw4FmshTFfY4UUQxl0drmN1xX6TcNaMRYTeAqdh2Xz"
"bzS1tNvMJMnwwftFr/hjVTrvBWK0WfRbtB9DpjEp+vnYIwh1jTZbv9FhOuu4bD5pD2Uj6kRbK8HFKJJo"
"Dj8E8C9wgIFS2pO1h8oHCuK0YMVTkJfZ1q2zZeU5+6JphxjJa7fqep9Z39g=";
char vds::cert_control::common_storage_certificate_[1821] =
"MIIFTTCCAzWgAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA5MTUxNzE5MzVaFw0xOTA5MTUxNzE5"
"MzVaMDgxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRcwFQYDVQQDDA5Db21tb24gU3RvcmFn"
"ZTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAKzRMWiFdbsUIYbm8fSV+3beCOFD6rVcXLpN"
"om35Px/m/Vkb+Ii8ECVUNn6zid5FWGPX7nW3YQoqUR4nZGhERl8CyZaeDK1U50HP42+jtgPXFuWttAcN"
"OXWi41YLkgPl9hd+ISCiMfaJygaiXiqUz9JbhZ0r3mrNdQqrWl4Jtyzv3VEGeh54mxgOfcCKE1mcPchK"
"LkskWUxa8bO7Tsf1IxQjgnnKBKupwO334ov1kK0ImKr94BwhbZPq5+aNQp/jC7oMnSX+OBOvxWW/ttCi"
"jpXVNo0+ap0dcLI/JWRTGt9N32jn9wBuOIPLKr9pDxBNZ1QaO1oksA1z3E1dUkGAkibcnKntn2OLujKJ"
"ahchaMfi7sidL3u87NcPueZMgllqwM/uThG69XvFVUW7uW70w4x3W/n+MRBNfzs6Xx2XGAxpxoKp0Eea"
"7jN/WM7QlKlbOX+DjRfzrRnsmPgvINksLDpzDb/nTvaASJMTLCzn5PsEoiy6ia/0qozq6Lw4k6kyduDJ"
"0YLoGE0woYY2ALjufBpM0xByzS9s7sp/vvMcBBjv31roWofWBO8lbPmIEhKHeuBF/m6WR0aY+d8a0Mp0"
"cXgqJm9t1/17z8ifPQ+KhApc83/rnvFv155Q8Mrk+TByW4WeFwVlMklzkDlxcnpsj5PyUurQfs/Fe0y1"
"AHJxGeDfAgMBAAGjYDBeMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMBEGCWCGSAGG+EIB"
"AQQEAwICBDAoBglghkgBhvhCAQ0EGxYZZXhhbXBsZSBjb21tZW50IGV4dGVuc2lvbjANBgkqhkiG9w0B"
"AQsFAAOCAgEAScaVE4OKHqP718+ZlQd26YbaL4ZH/kd4PQUEd5MlUkEAlQN8whn7PU1f90SRY9mb+tuP"
"5r/2MSyWrh4Xx7hnNCuMEo2LlIrJPJwzxko53yVHl4sqx+qZwx1kRe5ZihmsVM2zTDvwt9yoA7mnuNzM"
"eh4/5AyHsFQZxrkPJk0jM77v0Gfv9aWCTHSBowuFjBfBhWrb9xpqZtRkzPJH8aefOgwhJCRHxSvxR8JP"
"/oc+DNwgF75gUNS3979tfhDuTC9cm2BBkCyPjnaaHUOX6NnfSDS/nJoYj2d/dxwqvYT0qjhtHQ9YONxn"
"KPGnXf1bvkV91RiV7VJqXGmheng/XRegIpa8IodHYdaX5R2sIXvivto5HjICAk627InCyzpZrTHJ59Yu"
"yhY+g30mTGg+VlEwi/t92ouLYq9mi1db4V9rVDCAyFrG/9cb5Vs3drRkC3XGKovIsm5CYGLo0QpGMrdg"
"jeHBwXYNUTS2kGqisW1rKYuDu8PsWysiNENu4zlG4u7Kaq/gqlvk+gS4SeFe+vJk211/fHVJm1WJSb9l"
"aUnmoD92d/HYPenVWfpbEDx3g9SpGrf0TfW+5CJOf9IVF2gI7kFXoPI+vB6NwkJ3gDZLjAd4F7KK4G5/"
"5SQBH5hmLRZFhTKNeD9ytdHLr6Hz9FbzVyJFSACLscSXeFa1WzEo/lQ=";
char vds::cert_control::common_storage_private_key_[3137] =
"MIIJKQIBAAKCAgEArNExaIV1uxQhhubx9JX7dt4I4UPqtVxcuk2ibfk/H+b9WRv4iLwQJVQ2frOJ3kVY"
"Y9fudbdhCipRHidkaERGXwLJlp4MrVTnQc/jb6O2A9cW5a20Bw05daLjVguSA+X2F34hIKIx9onKBqJe"
"KpTP0luFnSveas11CqtaXgm3LO/dUQZ6HnibGA59wIoTWZw9yEouSyRZTFrxs7tOx/UjFCOCecoEq6nA"
"7ffii/WQrQiYqv3gHCFtk+rn5o1Cn+MLugydJf44E6/FZb+20KKOldU2jT5qnR1wsj8lZFMa303faOf3"
"AG44g8sqv2kPEE1nVBo7WiSwDXPcTV1SQYCSJtycqe2fY4u6MolqFyFox+LuyJ0ve7zs1w+55kyCWWrA"
"z+5OEbr1e8VVRbu5bvTDjHdb+f4xEE1/OzpfHZcYDGnGgqnQR5ruM39YztCUqVs5f4ONF/OtGeyY+C8g"
"2SwsOnMNv+dO9oBIkxMsLOfk+wSiLLqJr/SqjOrovDiTqTJ24MnRgugYTTChhjYAuO58GkzTEHLNL2zu"
"yn++8xwEGO/fWuhah9YE7yVs+YgSEod64EX+bpZHRpj53xrQynRxeComb23X/XvPyJ89D4qEClzzf+ue"
"8W/XnlDwyuT5MHJbhZ4XBWUySXOQOXFyemyPk/JS6tB+z8V7TLUAcnEZ4N8CAwEAAQKCAgEAqEa+zJ7/"
"552Pk/N3cIlGphVIIklm1xhLtqF344wK+n6K/WTuGf693gtlUPRz7/ooOnh9WySoGiTgffMvoxOwTj3B"
"m6bkgzckRAIw3XlU9+JOCnGc1iz7YV+t7BAyKxeWDIQlr8gfzIeZL+KUvPPQdW3m6gdYeTj8mdoGkYD2"
"W4xpPceHy/n2U3Nw4bXRZ7Vqt4f8EfEf/uijaDagXLaHZMm69+pNp/YQxedVMqF4MfUn64Bv5BRsxda7"
"3tR4JoYH2i8EKoWjhw/lr9EBK6DOJHnLt7cqSzjqjrZMK6d8iCoku9r7ufCZmUOQqXecUmx8i/lLB9in"
"kUHbJ34SZXPTWugwWVJHSz+08DOj2IzzNOyAzseGmbuLaxa54HkjfdpjE9bU6L6Vv5Dhkl6fbYQrrfMM"
"JtfDWGBnuUIUeg31rkUI1Av2vcRPae1wP0385If3fQEdgH9xl+gnIQ/p37OLHH+L50u6UArkI0UeZhPh"
"zdcDf30X75buzsL2eCaF1A4vLx8zoOVcUzn0wCsnr0XMz1qT2WkyGGWRkO5DEoZnYcVbzVM54Rn6wTQI"
"IOQiIt4k4h8HznpA4vh1vMIDQvvQnWBIGvbAnAcenwJ2g2tbEvvSoM+533NxupXVTmjj0Q1ci2oVPY8V"
"0cUSzy3ZlxmIbeniRD5o7Jq0Huzf+HUkESkCggEBANjSR+5qr1HhhsghT4LfLQ8aOw6WpHxL3QHy/yu0"
"8w5FIVKMU3klE86oc67hPrN0lMW4O6FvyzjeYzJGUia5lDYV2lqoFBQbQY461hnrA++mBI4cvDXVwcTK"
"xFYAnrvNFSIKGh0+F+qr9OXlrNNdvxG/2TyPOXnnw0ZD4N3yBX60uqD7gLoEj8pEqPuKoglBgCtL3d46"
"tK58URob2vnOTuDTTRnZ2bVLtQZzzxdYN1UUAxflJrCqKYfEn/7ZhDh/uKVVZMXmOPXWaOsZdFTadpB1"
"nMGVEjc0QpxURbsLMKkPtmjF9ZZpAWYxssKVvs+uPdcdNQ3sxQtTZOVsYnAbA6sCggEBAMwLXVZAQ2ho"
"gMfNjgeWDaP4EQUKuZkgxpGDjzOwZ3DcwPY7OVrdUZ7XM7c4x3Y4e2loCyDpyVHgf0mzFYoVnCm6mAbn"
"sBgzEvIjq0XR2COvKAtutQci/l50jvzH/aIuLhhfdzVJD5G3PwdfA674nnlrbKeLeapPCc43GFmyYKX3"
"L2WMaEmFAUE/X9q1hu/2r2UXMaEibNlfhoL5T/YK9fZ1LNDw3f6oW4fJdc8kj4h1plBWiCiP0XaaLWpD"
"oIfmyJ4WcwOmtSPT68rSk5eVX6Ls1mjxEEQHw6v6mFq+8ogCT1Hw1ke46itVYv0+6KqjS5QfYox+VTdK"
"azPifezY450CggEBAIyNpDuVxRkbTbgmf9iivB7yLh6C4RYCR76fu3Kl1noFrlhIx0PdGT36TSOsuGy0"
"wkx55Y39Xov6/er1UwPvgXieKfJiysFN+e0tjuU2SFLuGD3XutpbO0VK/IOT21J625w5WDxkqDRkTFlL"
"zgEm4FmtODHd1rCD4T6HehnmblF47xK0dvofQucoIzMM54WAhImMoMMS41XJGPoH3KfbaZ6s22pKoqq3"
"7R3FrRXBBf2M/4RZ8/7KECoG+IDImMHXaecjuDPhzkLvPwxIb/UhuYJj+74x2KEYq6WTq7BYzK6LlP/O"
"dCmQ4GtTBELHt+4BUe1E7cOdHQ7+fIWNATMTEAkCggEAX/1ssHnU/FZFVzmlen4l9qBp6VRNUfNwVt5s"
"TOXNE/RWNsBP0CZUEoStCKIGe+BAdqsCqT4yfnk9iH9eP7LA34SvpnfyiL/mRtFu7o60vrzU+uPe9/eH"
"cQomhlbTRCuzdbXbj87KK0Pou+AADJ1beEWIPR3JY8yBEWkr5G6dNxadTomKOiP/HiHhRzOkwljr1IPf"
"zi0yxeRBlHYTHu2zHnmrVrQpy3aKcMvex08s0wZ98kdzEJnv8XrAqMAd0jFI6c4n12zixHMa9zSvSF12"
"O2Bt0bqua0K30701VWOFF2T2ydaiB+W/DBnEUtz4eeYOVSWap/t6vWMeIa+IjBbQpQKCAQBIy8XOx46k"
"bRyPF61FrCPeWqr93Z1PNHwipF4rqXRYkj07+cT2yLZPokVBTZNGPdLG1rSPv3ODhvanqDjcZJ590UWX"
"4K+Er+ggwgL45Jmd/yY+/efYMkAsTz4MAGcU1mfYkQ7C9ejnlcxYLDuyEL0jM1VgvJ5r9gfmEZ7EFPNI"
"EYHNJFtMCz78AILl9iEw8WMxUcw8TXofFkVF3v9LR48wnWPjEmdyXOL8IS36Y9OpicCHzlY3ug9q5TV9"
"GRbfJSydxAxWX2tdUsygbiUnfWQKMwtwZun3RyDtw3H8f36zJie95QjN7qL8xYUjW+xNcSE1aqr0sV8m"
"w4k6vuQkM6Qs";

/*
 * User: user_id -> certificate (object_id, user_id, parent_id)
 *
 *
 */

//static vds::crypto_service::certificate_extension_type id_extension_type()
//{
//  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
//      "1.2.3.4",
//      "VDS Identifier",
//      "VDS Identifier");
//
//  return result;
//}
//
//static vds::crypto_service::certificate_extension_type parent_id_extension_type()
//{
//  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
//      "1.2.3.5",
//      "VDS Parent Identifier",
//      "VDS Parent Identifier");
//
//  return result;
//}
//
//static vds::crypto_service::certificate_extension_type user_id_extension_type()
//{
//  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
//      "1.2.3.6",
//      "VDS User Identifier",
//      "VDS User Identifier");
//
//  return result;
//}
//
//static vds::crypto_service::certificate_extension_type parent_user_id_extension_type()
//{
//	static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
//		"1.2.3.7",
//		"Parent VDS User Identifier",
//		"Parent VDS User Identifier");
//
//	return result;
//}
//
//static vds::guid certificate_parent_id(const vds::certificate & cert)
//{
//  return vds::guid::parse(cert.get_extension(cert.extension_by_NID(parent_id_extension_type())).value);
//}

vds::certificate vds::_cert_control::create_root(
    const std::string &name,
    const vds::asymmetric_private_key &private_key) {

  certificate::create_options local_user_options;
  local_user_options.country = "RU";
  local_user_options.organization = "IVySoft";
  local_user_options.name = name;

  //local_user_options.extensions.push_back(
  //    certificate_extension(id_extension_type(), guid::new_guid().str()));

  //local_user_options.extensions.push_back(
  //    certificate_extension(user_id_extension_type(), user_id.str()));

  asymmetric_public_key cert_pkey(private_key);
  return certificate::create_new(cert_pkey, private_key, local_user_options);
}

vds::certificate vds::_cert_control::create_user_cert(
  const std::string & name,
  const vds::asymmetric_private_key & private_key,
  const certificate & user_cert,
  const asymmetric_private_key & user_private_key) {
  certificate::create_options local_user_options;
  local_user_options.country = "RU";
  local_user_options.organization = "IVySoft";
  local_user_options.name = name;
  local_user_options.ca_certificate = &user_cert;
  local_user_options.ca_certificate_private_key = &user_private_key;

  //local_user_options.extensions.push_back(
  //    certificate_extension(id_extension_type(), object_id.str()));

  //local_user_options.extensions.push_back(
  //    certificate_extension(user_id_extension_type(), user_id.str()));

  //local_user_options.extensions.push_back(
  //    certificate_extension(parent_id_extension_type(), cert_control::get_id(user_cert).str()));

  //local_user_options.extensions.push_back(
	 // certificate_extension(parent_user_id_extension_type(), cert_control::get_user_id(user_cert).str()));

  asymmetric_public_key cert_pkey(private_key);
  return certificate::create_new(cert_pkey, private_key, local_user_options);
}

vds::certificate vds::_cert_control::create_cert(
	const std::string & name,
	const vds::asymmetric_private_key & private_key,
	const certificate & user_cert,
	const asymmetric_private_key & user_private_key) {
	certificate::create_options local_user_options;
	local_user_options.country = "RU";
	local_user_options.organization = "IVySoft";
	local_user_options.name = name;
	local_user_options.ca_certificate = &user_cert;
	local_user_options.ca_certificate_private_key = &user_private_key;

	//local_user_options.extensions.push_back(
	//	certificate_extension(id_extension_type(), object_id.str()));

	//local_user_options.extensions.push_back(
	//	certificate_extension(parent_id_extension_type(), cert_control::get_id(user_cert).str()));

	//local_user_options.extensions.push_back(
	//	certificate_extension(parent_user_id_extension_type(), cert_control::get_user_id(user_cert).str()));

	asymmetric_public_key cert_pkey(private_key);
	return certificate::create_new(cert_pkey, private_key, local_user_options);
}

//vds::guid vds::cert_control::get_id(const vds::certificate &cert) {
//  return vds::guid::parse(cert.get_extension(cert.extension_by_NID(id_extension_type())).value);
//}
//
//vds::guid vds::cert_control::get_user_id(const vds::certificate &cert) {
//  return vds::guid::parse(cert.get_extension(cert.extension_by_NID(user_id_extension_type())).value);
//}
//
//vds::guid vds::cert_control::get_parent_id(const vds::certificate &cert) {
//  auto parent = cert.get_extension(cert.extension_by_NID(parent_id_extension_type())).value;
//  if(parent.empty()){
//    return vds::guid();
//  }
//  else {
//    return vds::guid::parse(parent);
//  }
//}
//
//vds::guid vds::cert_control::get_parent_user_id(const vds::certificate &cert) {
//	auto parent = cert.get_extension(cert.extension_by_NID(parent_user_id_extension_type())).value;
//	if (parent.empty()) {
//		return vds::guid();
//	}
//	else {
//		return vds::guid::parse(parent);
//	}
//}
//

void vds::cert_control::private_info_t::genereate_all() {
  this->root_private_key_ = std::make_shared<asymmetric_private_key>(asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->common_news_write_private_key_ = std::make_shared<asymmetric_private_key>(asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->common_news_admin_private_key_ = std::make_shared<asymmetric_private_key>(asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
}

static void save_certificate(char (&cert_storage)[1821], const vds::certificate & cert) {
  const auto cert_storage_str = vds::base64::from_bytes(cert.der());
  vds_assert(sizeof(cert_storage) > cert_storage_str.length());
  strcpy(cert_storage, cert_storage_str.c_str());
}

static void save_private_key(char (&private_key_storage)[3137], const vds::asymmetric_private_key & private_key) {
  const auto private_key_str = vds::base64::from_bytes(private_key.der(std::string()));
  vds_assert(sizeof(private_key_storage) > private_key_str.length());
  strcpy(private_key_storage, private_key_str.c_str());
}

void vds::cert_control::genereate_all(
  const std::string& root_login,
  const std::string& root_password,
  const private_info_t & private_info) {

  const auto root_user_cert = _cert_control::create_root(
    root_login,
    *private_info.root_private_key_);
  save_certificate(root_certificate_, root_user_cert);

  //
  const auto common_news_read_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  save_private_key(common_news_read_private_key_, common_news_read_private_key);
  const auto common_news_read_certificate = _cert_control::create_cert(
    "Common News Read",
    common_news_read_private_key,
    root_user_cert,
    *private_info.root_private_key_);
  save_certificate(common_news_read_certificate_, common_news_read_certificate);

  const auto common_news_write_certificate = _cert_control::create_cert(
    "Common News Write",
    *private_info.common_news_write_private_key_,
    root_user_cert,
    *private_info.root_private_key_);
  save_certificate(common_news_write_certificate_, common_news_write_certificate);

  const auto common_news_admin_certificate = _cert_control::create_cert(
    "Common News Admin",
    *private_info.common_news_admin_private_key_,
    root_user_cert,
    *private_info.root_private_key_);
  save_certificate(common_news_admin_certificate_, common_news_admin_certificate);

  //
  const auto common_storage_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  save_private_key(common_storage_private_key_, common_storage_private_key);
  const auto common_storage_certificate = _cert_control::create_cert(
    "Common Storage",
    common_storage_private_key,
    root_user_cert,
    *private_info.root_private_key_);
  save_certificate(common_storage_certificate_, common_storage_certificate);
}
