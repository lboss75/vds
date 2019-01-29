#include "stdafx.h"
#include "private/cert_control_p.h"
#include "cert_control.h"

char vds::cert_control::root_certificate_[1821] =
"MIIFTzCCAzegAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODExMTEwNzQ1MTRaFw0xOTExMTEwNzQ1"
"MTRaMDoxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRkwFwYDVQQDDBB2YWRpbUBpdi1zb2Z0"
"LnJ1MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAwY6K94OcndV7k8y9oFPmWI2VBZ1T9zDo"
"CN2sS0nk2eza/9BWjZtNgmgphR84NrnaaucjSvSgtcWAGLExnQZJxLdMYah0Ru6XPkpgDMJZY9f9NvLr"
"Ryupxtux8Ko/TVpdruzleMxq+NAWz8ckyrFK8xgQcg99bEcmxXcsGu9lOWKQ2brrFhletc1RZg1TXovq"
"s69IWtQMyizJQLOa9fHuISjgRY+EK0uPjkoWSAB6amhc+/PClsGYU+wvpvr6zKIOmrpilz1FAQlpjbsB"
"O46+raZCULcvJA4BdvCWHbYbEjq0kIAuSxqnp069MLSPsmZdsMSMvZyX8GQxCu1/xX5IHuKL/kpZDEGo"
"xyGuO/sl/osz/JQRfUxQPOCkgr/Yo0tu9eVPGSPexNkFVVAoe4OoMm6IkOjmlYb9VYkv+QMw5vBdC4E4"
"q7h031ibNmVYSL8osotf3XnCuZhPc28Mh6B1s42DYlZW9N2QV98uS1ft7773E/YMjROVJwvHotoP8ZVR"
"M62H1bsCLWmfgAc/M8ejLPnsTQkOXi+jBNV40Ry2NQkQ+2zIVTRzFVTajldcy9znggx1Y6TCXYAAtsFW"
"xacSj2qTb1aCDGh95QYlScf0nEIeKjaWQO/+1xEumLiAAtSnt9yTk7c5xqwsXH34SaW/tT7nc7WeJ/W9"
"L85hveY5MCkCAwEAAaNgMF4wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwEQYJYIZIAYb4"
"QgEBBAQDAgIEMCgGCWCGSAGG+EIBDQQbFhlleGFtcGxlIGNvbW1lbnQgZXh0ZW5zaW9uMA0GCSqGSIb3"
"DQEBCwUAA4ICAQBxUpzUqpzLGR88vy19Z5t989f0Z9bkQ+dnb5H8kfHjSrFkC8B6USU4REf9f6BAUOOa"
"uKp0pouz4q5JmYMUQ1jss8Ylg+Uz8hnj/B2qZgzMAyYBIwYA3jTAnYwi7MBhNPC7fKnwQ7EkuG30Klqz"
"2K2d7v4NVLszpyDOklY7LLfltRIdG5vMqJ+ohxCidNhwAkzkDMiwDFPKmgyqlpzrdKm2PLr1fsUVleVE"
"lRfmTwohrjJU062r5VB+kScUm+pdpRV0kXUP3JuHLiSn3pOv6vju2knC7DvnIjbJnzE8Xb+PKMGAggL/"
"l333RPDgAFqhAhtBEXkCwaFHmgy5mXLwCGZA/ZymmnwkL2TBlnpNGQ9P6ZTSEIr9sw2NHRUGctp/iVWm"
"gL/SzzoroRl+17E2mw+gazGdrwwjWw2X+ttFCUKg0kX8KMV+WwOdUnGTh3Emzh0/4A92wN6ULI81ZE7B"
"n8Im6lwFbR/Xmq1NS7gxXONCOlGCoJ4Jmt3OhL8ebF6b4H6XFBMcatu04CH7hTyDcQV2xU1NQeVgKZfu"
"vKn8CsFDHkFSnV++XfHG6lNFbCyDUScOABepfx56UU0e5s29YolpqOGno8TBQUMiODmuVmR+2hmjlHfZ"
"jJM7LcOPJfGYmDf1V7GaUmtxmdirab3DZIaxsoKjD67i7HkDZvSE8s5w1A==";
char vds::cert_control::common_news_read_certificate_[1821] =
"MIIFTzCCAzegAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODExMTEwNzQ1MTVaFw0xOTExMTEwNzQ1"
"MTVaMDoxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRkwFwYDVQQDDBBDb21tb24gTmV3cyBS"
"ZWFkMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEArp2Yw3gm4iNkr82rRNeb//nqv+Fj2h8b"
"P1eOPbnVQJPAxR4jPUrqyk+Y3EDbGS7kY6B4MSs0NuqQaxpYGOAc+HTS/a7f2Manh79+aOiFwJNJjXMc"
"rpOyfFeAh827Ek8XWek/S26PPlHGzQGQMLzmms1dfYWChn6ghK9G0ORxqKG/3yUE0bGyTJsKe+n+9JAO"
"G10L9JuwrpZl76YFfjjHKNps//3IimbW1J+KTI0pvIbYiLVXF2r3Tpq9kvtQ/3M6/7d80ZUpf2yQS/Zy"
"l23MqpTfV19/NCcxVfUu9XIcSdfFd4jFPXzZdfDAlBuEVqoLCN5DWaEklzoEZuQ/xoztRmwIgAbwW4au"
"CKmPQaoMR8Rt2F0WGCPBAFiOoB8o6ThYWuapcxelGhjx/GJN7KKrrABBajff4M80D0mS63uj9y+3ELi5"
"I6z8OtEMF1+ib0whTGdh/lW9VpGzxo9Bp5dVLAwlfUD7R1peYqDbYRyXGA5OH3A/KINo0A1N5oggNJqM"
"bcLFjF743T8SbdFHRKr+FVDF3L3bMHgtvzn0n+XrfXg/2QeDIhpyvaQyBJk4LQnfzlkedCPNDbrxRHyd"
"8gOr2SJEUyzrZscDVOnWVKKK78ColBUyBQDpWn3JHDw7rn2uTgkpbaLngT6JMBqRShcsckwdGPKT+asi"
"qHr2JlPjvJMCAwEAAaNgMF4wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwEQYJYIZIAYb4"
"QgEBBAQDAgIEMCgGCWCGSAGG+EIBDQQbFhlleGFtcGxlIGNvbW1lbnQgZXh0ZW5zaW9uMA0GCSqGSIb3"
"DQEBCwUAA4ICAQCcGdw8OGYG1DbYsHHci9kxBhXukKmubw4gESFTo8wS87tAh8kgmhKULCug6NsMC0A5"
"UMOlqwNWDzHasqa1mheKR4nJqKysej0EWmEvkHAykmDGDiVw4MMn1IW2Vs5gJ1Dl1hvKKWOLzDM56SXw"
"/TVTt2rAt49YgPsxL/9+WUHaxiRq++vA8OJgfIbco/lCYEH3uWCPSQzqRinRIQv5N6XtuG3Sws3eFW7X"
"tf4QWyWF25CgDrIEPxtwobX1w7FrCxbW3Tz65GABhPNLHraRlA6Pl8cC77l0ySpX7VhcLP+arcwz87bT"
"s/mwSjSshl93GchCAIiYl2q9N1nPONE/WopFJCYYM1p1wJNu9qGEFI1klyFAwmBhGxFcZmvj9jmsvcKw"
"EDOJKMEdF/mcg0J9TGzBM8BljUZbN8D57SI3U10SYU4NGAdt5YbVxhzjFmIw8DWxv6O5Y9+fQVyzmkJV"
"wBRlVmCy0b0HrAcQu4ezvEujD8htilG+IpjQj3NKof1UnzmnbsN0iKGDBcVZ0We4bvilcc6y/rv4Vdfg"
"xFdwbOc+H+IqTQ/SGnbFoTqCX59KOUVnVJBX25iK4c9npqGwDh43cXN+r5fufbkczqvC/rXWTL+xv2PT"
"po0BF2bMl64vVYPZoMo80j9IZkcfwkvFIRzzAxv/dVOyHFKYQks9zDA+xw==";
char vds::cert_control::common_news_read_private_key_[3137] =
"MIIJKQIBAAKCAgEArp2Yw3gm4iNkr82rRNeb//nqv+Fj2h8bP1eOPbnVQJPAxR4jPUrqyk+Y3EDbGS7k"
"Y6B4MSs0NuqQaxpYGOAc+HTS/a7f2Manh79+aOiFwJNJjXMcrpOyfFeAh827Ek8XWek/S26PPlHGzQGQ"
"MLzmms1dfYWChn6ghK9G0ORxqKG/3yUE0bGyTJsKe+n+9JAOG10L9JuwrpZl76YFfjjHKNps//3IimbW"
"1J+KTI0pvIbYiLVXF2r3Tpq9kvtQ/3M6/7d80ZUpf2yQS/Zyl23MqpTfV19/NCcxVfUu9XIcSdfFd4jF"
"PXzZdfDAlBuEVqoLCN5DWaEklzoEZuQ/xoztRmwIgAbwW4auCKmPQaoMR8Rt2F0WGCPBAFiOoB8o6ThY"
"WuapcxelGhjx/GJN7KKrrABBajff4M80D0mS63uj9y+3ELi5I6z8OtEMF1+ib0whTGdh/lW9VpGzxo9B"
"p5dVLAwlfUD7R1peYqDbYRyXGA5OH3A/KINo0A1N5oggNJqMbcLFjF743T8SbdFHRKr+FVDF3L3bMHgt"
"vzn0n+XrfXg/2QeDIhpyvaQyBJk4LQnfzlkedCPNDbrxRHyd8gOr2SJEUyzrZscDVOnWVKKK78ColBUy"
"BQDpWn3JHDw7rn2uTgkpbaLngT6JMBqRShcsckwdGPKT+asiqHr2JlPjvJMCAwEAAQKCAgEAk0geiBEa"
"Sverf5Wsdl6eGGTGGp6TyHEgXGcPdQT/2H5HxHvNIW/FZmuI9Y5Gv5EL1vSNLHvSiNeEhLItyJ2QEgkH"
"xi3wZTn6KMfeacJmvq39BjAn6yBtFfAAW4ut1J2dhpr0Zj2U2O3FDznYUyOiJsQ9rRglpie8YO4ufxeQ"
"vrnfyunAjNajG3ZSe349LC8tlur+oI2Jk1kMFf0k+PgPFGhYM/vtYL+pSKJg1nzizT+3+GdsVDZG7qga"
"apVMFostIm+z+lskK3R4qXnv4cYdMWxsjZfhBS5hsK0jtKdRMP+scUyNgXWDPrWY2dPz5d17fTlr+Xew"
"E1Y+TEMNf/1yS+SoXEZe0Cl7HKUO/WlWF2yzZV/kjJHpLMQK3Z6g5vxeTX8AdDVNdNZexmxAeJcFUe/1"
"z7RNvUN2FUqQ9WndLEgUl+lP7Rlc9rAOikta8+RQSFLUO3N7FwFq3tXgDvnJ5QKU4H5g/dISwk4A3Rf/"
"6qJ+uZhv+viBBKe/H/Fnm5cxB4MMV8P9nFcklFkQ0WqYm8f5iyBgFzRSgm8EbwPiKcWY//5BItCgUIUI"
"v8B/JLIKRoO/N8vv6a09eXNRKJwXypsjISiZyb0SA+b+funEVH+Rk8g6d/hBOKcJ9ao1J1ceLDMh9v1u"
"b5HzuC8Cw33QDNFykcbA1Yr0E/xJplph+NECggEBAN/ULnX1XyKpjHFajUTI37v+Hkjhc4g4UvxNRUz7"
"vzJSy16DRUvc1w0ta1FkwXhsHwfRqCrwMaFSoq5egRJmvCSDkYgFAsGSQbp7+UFIyzziJMK2zyrGxzYe"
"4O2ef00ZlpSIkVzyfHeob2W7GhCXqqbEds3pGq+cVEyIGVAGEZsQEcFVdNGMdFnGtwOaVjjNMikDy5K9"
"Sa8T4amBEiKhkahlhEIKTo0qL1ab4dAxEfm3bHJie+8mep2kP2Rw7hOJ/Tvrffh7CZJP6Z5MZxspVEQJ"
"o6s1RuWEQW67kqwhgLQGnasR1FcwfyNjTDAtBZuT45aj6EmP77ks2pLfFjmHCLcCggEBAMe2m1GFACZa"
"R/YE9JmibwzSrObp2RXhZbrE7UwE694mowqZZ73vzUlFaRXX7XisHl0cADWJD2+9BSAIq4fD0fBdQq4k"
"s0tqybA36VCXm+TE48958df4e/qM9UM9yYAk+I/mMLjf4VBa2DpXt2dM8xzb5eYQWVAIoJOHCFs3q2P6"
"hH/6zy9LOxJcmf/YBsyeUcDgOE1J+S2zLi9vLouSGogthHwcCo1oaax9ueS8gHhkEOJTn3VeFwNe0/tx"
"ApR2MFI1GPzHDFLehXATnKr/i+O8xqH+vgnwdDBpqnrxhQJhJC+tnCt8HTJHeYQ3ExWcdDY7KgpzuzLC"
"w8y9gyPo9wUCggEARBSn5R0zSLnFjBz85zUqSGYtFAHvQDnRlAh00cupBkeEsETBaSfNCacNA37gEh7g"
"+WHeAuF4VxdpgBwiqlQxWfHi7DqJ8hLohy56TE818lje0ZMFRH+dzGpsBBq3od3snVVE0b63+TV0XL8i"
"WHWpZHxRnqQOh+fHBFhJSrvt0vp6TBIQLETKhxwYQrJJE5Hmde8/lmyY4vXrJ8GooHwAb/Yg2m2EIw/3"
"wiI79zVVwFz4UXp/M+jCVvzEg2qOH8dTDUy9zjyuVqCT8KBQpERX6zH7ZHIaFGm79VB8dnriSN/SSVxs"
"9/A6W6syW1z2XbIIAaPtXaK4SdzIxu+wNONL1QKCAQEAwBIR/GuyQjBvpt8LMzqyHJ39JbNfUZjN0rsU"
"ERlmkueLxGTXDiezwjUcJKLfQ0Adj6SfWt1vWCJs7GsKNO8SWF4JBBxNx0lzoaOUIys8Bd+JzP9VpUTY"
"T5YDR2EECtcDqrlSwGqocjR8Ylf8DkBGS8n8p6+vhbZWYKvsUHYQwn2rsr3PRofOzxD6FsF3gJD+xUWf"
"QE3VdKsvzvTZyeeA1tkX4pjBWVBC2V6tSpwz3hU228RaxnDwJxCrRNZiCdBFFrr0Rh9U7doOcUSbVyWg"
"Ly1dx2yM7lgwFmN5TvY62pf8SlJosLWUCsgMySiClTU72vA5tYWqUt5Xmu/kUMP71QKCAQBDlPvru7BE"
"f3IHCJcajZGSoAx1AfvvCEQlQ0e0TDRhW73Pr2sbVP1IWgzJ4lYepg3Aufnj0ma6pkIAMnoMdulxoXp6"
"EyawPaXdMVIfD//7X5No9hG0l0tvThcvrBkXJybjww+zj7+V0KGGi+hRbb1wgj2FgmVMjmobK6s5D85N"
"AEfp/iVJVn08U1EdukIkZKI74cqpOtDV4nItYcuvNE6/wAzBJA72eEgPnTC9szpd0/IqGt0w3/huXAko"
"kU9VO7Wse3enDygPZ28EVLMnI3mUKASgJ/5dQvJL7s5jiEgOFLFVB4/8SKCWg5wP99wI9kmFCZMWaJLD"
"1kULA0W7iJ+6";
char vds::cert_control::common_news_write_certificate_[1821] =
"MIIFUDCCAzigAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODExMTEwNzQ1MTVaFw0xOTExMTEwNzQ1"
"MTVaMDsxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRowGAYDVQQDDBFDb21tb24gTmV3cyBX"
"cml0ZTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBALAtOb8JDZW4dR1TuvS3G2ulRBpRkkvA"
"eRTSqE9ohBhdxzPsYXjS5rlbrqOGd4zj5PsuDgPalqzqphkuCgjzLfPwWyNW+mo7ZpBqS25IPWGEg+d4"
"k6LheP8i5kqv+LGICjUJ76hgJFRz4M2857V99SsZyL9kUnBOUy/Wdq4Ae8JerjVIok/wZ+A23Cl32RUG"
"3At7Hvun4/70Cg0yoI+jJKUqog0WtCGlVtUf+cMkpp2NFrx9cFhsPcGBJwDcCer2qACv7xaAr50WvS5k"
"5peJ+1AZiAdPEW8f8ITG7RF2a+mVGXn5QnikMiDQQe5WBrC/9X1vKbiPu83sbclBxY+aeJ6+Ky7rkc+v"
"R2hxjOP3mYB0mxvjm9f2Ev1twBbZ5sFe1a08Fdr8IjUh8JTZVg6Hv8AnT/1W3CxOK+GnEgF+dUeQM3KI"
"AvJp5vV0cxR0xCRI4/iV8SP6jOENxMTJmFJLqbgjtFBZOAk004t7e+gSOx873V6OJmWyKUIGgL0YG2cH"
"XndK+ccISVD82b8XIUr+MQiQiXPh5iw9vejzO9JFpTRav7Upr9tgshaUJydufQCi7Swb06irYcTWXiNL"
"B5Hxe6guYUhJopUoH7vDu9oB/WYI2v6fEcBTZNgeTGNBhqc/qTuU+8LX0iZ5jAeCzf/1pj3OGcY/+b+h"
"mkafv6P7qNrBAgMBAAGjYDBeMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMBEGCWCGSAGG"
"+EIBAQQEAwICBDAoBglghkgBhvhCAQ0EGxYZZXhhbXBsZSBjb21tZW50IGV4dGVuc2lvbjANBgkqhkiG"
"9w0BAQsFAAOCAgEAZ6nYlXzZiWEmJSE+MoMcJVsdfQBCmL2aDoIM7EELUqAtiYuuVkBNptuCLubPKFF+"
"tn9nOoO3Y4J/ZN6Hg4KWa2zKHPS5+8Mg2/NfIywTCuASZK4u0GjTeZ06QMlNf1vpjpqIguJZru5iuhyD"
"JjG4ll1EYua/DaPCeHvurdtvDF06HT9MQ+zmnXPGVyUysTynBo9pvB/++/B8FL36dMg2/XqsCzI0Hvm8"
"fkqfqIWPHfwO4oMx8T1mfg8gm7UPmSX5h90e5xthnMvqhZvi1N25ptRnjLu2hJv5Xfl/dcpA1JqK6yG9"
"YW1/HsrKsf1yzmG+jCbYawxcFQK3heAznO4PgwtzlNXlHbOu4Iujv9E/PJLnQllzm3ZK7xR8WphZc41f"
"CyVAPTLRnDZFggfeFDa4n+fxuw5etuLLRxjjYYc2UfVmctPocMIohu8YrpPotXxV4mmervQoOtHtGPWW"
"L96ieJ/bQtjxcYix4zFXpWjcpsxdZ5FYbAXso0p3GRaplZOOtg3FRz8aradLk8NLUqT5FozLiChtaqY6"
"mDl+Ji4OhT4Bo+emcGSnogrPyfF1i/lPiv/j0ORAmTUp9VCpcmVtqfyxmQVxfzGdbx04RSvhoR93J86n"
"bYPd46U9YZBzRtP/qwX8c6mMMDSdG9XgkEFfXtbha1/71WlJ3P+NShtselI=";
char vds::cert_control::common_news_admin_certificate_[1821] =
"MIIFUDCCAzigAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODExMTEwNzQ1MTVaFw0xOTExMTEwNzQ1"
"MTVaMDsxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRowGAYDVQQDDBFDb21tb24gTmV3cyBB"
"ZG1pbjCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAL4Fa0yjyFBtjQHeAmK6Zy13AMb1+4z5"
"cEiCkj7IGZY0IMxzrAl4cjmwmIlGUHQ4dkloj0y4Fpskt0FP4wwhQpxVQNcpFIAHGaAbojgj9QQ5RXD4"
"GfZJUDcnIZ1/mZ8cXCfvF5PQRhY6Zd7ml199/w4vWecqHsKcWGl1cWEJ3oiUaJKcMn0fAFgNhao2eeXO"
"rOmtDtfRELl1BC6UXvjbhj4j/OcIHOgj1WrwDnoh3i8h/JclfpItyrPDSitm5m/kn1xudRTW5c1lsZqJ"
"+VZVjHBYrTOslMsWdKzU+f/K7YJaEEH4EEythf9mwqWp5fomq2nGFGJTLCNmzArgQPpBRSWsWNcq49KJ"
"NkR2O5PBJQdMfdByGmqsSxdigNyY5BW/L7qWxVCJvHSKYuYzI6Jk0c4e5QEcyFBxO9XMcnjlfiiNC3Tf"
"karCcq6FrRbpsExp/wQuFK/xd5cdATzBpN+9vULfzAv9luy8l6ISZ6dezyNpO6efWs1187/LR3TjBp6g"
"JeIBgmHozkgoGO3ExNDy/EvrGp0Pxg0sbs42MoqHODesETz+iVaZjMZVBoaePnjgD+WsKOx8xY+bZAC7"
"oC03LIrUUyjr4bp89y9Ge6+75rc002YhyMY7WpZNCo2EPPJddcEdGhuNEq0gBGJKzc8tNU0aX/Lw9GPo"
"7DkDBHmfJrpRAgMBAAGjYDBeMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMBEGCWCGSAGG"
"+EIBAQQEAwICBDAoBglghkgBhvhCAQ0EGxYZZXhhbXBsZSBjb21tZW50IGV4dGVuc2lvbjANBgkqhkiG"
"9w0BAQsFAAOCAgEAET5fzIi18brVUsux8mT5tw6YUbbYv2o7VbQUL0VxDEj5V8ojEXI1CLqumJ9sL1hM"
"Urs5tmC4l7oStx5dhhZrxi/QESguuhdDsShtD9AbFlnbz9n4mwf+k5PtuS1CIvoJvQc0NzO09kq1f4dx"
"88b0TeHZ95NTTOYADEnsW48ZfTaGqK3/DnGRWuc23Zx3u69n++elR38azp9hf3E4YfCq7ktIQlLwN+xj"
"5Z+UxSat8KFkowsq7caLSzumlSzZUB+DdgBY/bDzMbNRklMZM1dBOp1TNokk/Kjs9yE67jSEDI2z/c4q"
"ecewIHGKjyuyiDBtePpBykFQHO+K4Xp7weTztwBPJwfrFHjxGm81Z+EvO/ByfYQHA6xfjb10F4fMpNkw"
"8F3UfXu+Q+0dVkmjv5Xkulm/Shs/44eRsYquRIyT+thFZ8Q8Rcn5nauPnSpRy1mupcYgKWEbnUmGR1bN"
"fo0IWXHj7ecloFxuWhjXnknpEyUw8WSgUlyAN0yNEXALSATKAz93fXhv5Ok/enypxljrY1I5BeTff5EV"
"mgcQAJO1vR0VHgo2HR2DQMhYpbxklVNX+/9d4k/XbqwKE3uRZ6SxJ3nupeAZ5iXAoqfmH+DbMVvvH5rb"
"X0oog9RnhlJMnfUZ2IUrHIEDKfMIeVcGQGGHkoImaYaHzC5BpHvkiZStcqY=";
char vds::cert_control::common_storage_certificate_[1821] =
"MIIFTTCCAzWgAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODExMTEwNzQ1MTZaFw0xOTExMTEwNzQ1"
"MTZaMDgxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRcwFQYDVQQDDA5Db21tb24gU3RvcmFn"
"ZTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAMJFOsHXY6kJH5WACSsSPv38giQUULccVcN2"
"PUdT639MIDnh7s2F3twssr1xK8g1yrwIEdmpDaaNJtTy3DCtp9DJ2CUPF6Q0XABewrimi6IWeFb/VxCQ"
"w7VgspRQlq/mYlW8Qiu9VhXP+ahMHKVg4ivOZqf2CofvHcxGfEvmDH2FlVm0VOSxEKZPW0l/WdthNESC"
"Xudln2JBYTp14JtIbY70mNxH5Cp87rORdeB2EHO3N5JPnYlIKvzDexADYifp6QsIXiL4sCroaMlCe5Rl"
"TYQULW2mNl82cNGKGgZGlcqFHlhrZ5teJLjkxC8fDmqqBaMgh+SuOQ/TEp0lgEL5Pkpk15E1tN57hT4J"
"EhszMnOFCleVRbhKSbOMsBMLXsxWy+plDV4qWF8MC0mIBEzT9JlALIJrVPjzjB7qYo+VE8rTiYqYe4x1"
"1zrhCOVgkkjYZ0XQw4DA6cwQqbjPlSqqeLiKuU1a1CofOiEsJ4wmZMlBpLyD9IoPjx/xZYWJ8j1K0tDU"
"xlYiorQPoT8Qs5Qt8lkryIAfTewtbB3RQ302Jwyte9clr2x3kc1kMQeSnw10v9bR1voOoABNYeMWjxVe"
"6woqn2UVqr+K7yxq4etsoLYTdgqi9FFOeleLRzEFFRY/dGJXZeYLeiEf8voJLqQKp2XRkbs0ySV7CCij"
"HtG8ElGVAgMBAAGjYDBeMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMBEGCWCGSAGG+EIB"
"AQQEAwICBDAoBglghkgBhvhCAQ0EGxYZZXhhbXBsZSBjb21tZW50IGV4dGVuc2lvbjANBgkqhkiG9w0B"
"AQsFAAOCAgEAiqlndSSGOeQ8Oh+1JTCrRmNKNZ4KjajEFlDj8N+7fxIFG1GNSggvryCk8/i00KouF5zB"
"8CqHgwIbfHc7XmYKxWBPzpNLazNppc4y9qD2V5YdIzYfNoyeOi/N+Oi2BPG5Gnp/EC5D6ZNDExovO/ot"
"uwP6+XmugbsAUmVLjWONhsMAR0WxSJrJsMTcH/coxjaX+M0mkRV+sDHBSi6hE5RT0AbwSR+zGxnbJlMR"
"H9m+mYr2xzSHlx/TB7rHsPO9IH7pIrHyT2e76nWaY61eic0NOpu1t9dF7SBWpJDQW5yIwYPP3Qybj5gD"
"PtDVUpKUdFf/4bUppIeq2bEUNAvTWNvEcWgcqwTT0Jf4cMZpQFbLHA0S2C8Kluau8rG212hsJNSN3kKB"
"C97go1QK5ettxCOLUQizOgppWhEwfO6kHhY/MNEDE0AGlxU0V68FlUegUdqTOuV5TM5XAKLQUrYjKs4Z"
"QoY484LYgS8vKXO1voyAheGFQhsF5phDgOdZ1EXjYFYEPcNK0y7dFOlGT24rRBfNuJzPkF1FlVVcjaVD"
"MsSTRnIyBTyg1R9KFlwS1IqIr2iKQlQMqWG1Q+aySWzOgKEX2kVtYmlbLoDPhbwvvzh9KGHB0jwkHS2C"
"YllsM6tkg72SKjyk+5VXH65HjineHhj0qbcWm4+a+RQOJjQFmsrYj+k=";
char vds::cert_control::common_storage_private_key_[3137] =
"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"
"6IQmk0tT+AzC";

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
vds::expected<vds::certificate> vds::_cert_control::create_root(
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

  GET_EXPECTED(cert_pkey, asymmetric_public_key::create(private_key));
  return certificate::create_new(cert_pkey, private_key, local_user_options);
}

vds::expected<vds::certificate> vds::_cert_control::create_user_cert(
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

  GET_EXPECTED(cert_pkey, asymmetric_public_key::create(private_key));
  return certificate::create_new(cert_pkey, private_key, local_user_options);
}

vds::expected<vds::certificate> vds::_cert_control::create_cert(
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

  GET_EXPECTED(cert_pkey, asymmetric_public_key::create(private_key));
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

vds::expected<void> vds::cert_control::private_info_t::genereate_all() {
  GET_EXPECTED(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->root_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));

  GET_EXPECTED_VALUE(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->common_news_write_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));

  GET_EXPECTED_VALUE(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->common_news_admin_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));

  return expected<void>();
}

static void save_certificate(char (&cert_storage)[1821], const vds::certificate & cert) {
  auto der = cert.der();
  if(der.has_error()) {
    throw std::runtime_error(der.error()->what());
  }

  auto cert_storage_str = vds::base64::from_bytes(der.value());
  vds_assert(sizeof(cert_storage) > cert_storage_str.length());
  strcpy(cert_storage, cert_storage_str.c_str());
}

static void save_private_key(char (&private_key_storage)[3137], const vds::asymmetric_private_key & private_key) {
  auto der = private_key.der(std::string());
  if (der.has_error()) {
    throw std::runtime_error(der.error()->what());
  }

  const auto private_key_str = vds::base64::from_bytes(der.value());
  vds_assert(sizeof(private_key_storage) > private_key_str.length());
  strcpy(private_key_storage, private_key_str.c_str());
}

vds::expected<void> vds::cert_control::genereate_all(
  const std::string& root_login,
  const std::string& root_password,
  const private_info_t & private_info) {

  GET_EXPECTED(root_user_cert, _cert_control::create_root(
    root_login,
    *private_info.root_private_key_));

  save_certificate(root_certificate_, root_user_cert);

  //
  GET_EXPECTED(common_news_read_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  save_private_key(common_news_read_private_key_, common_news_read_private_key);
  GET_EXPECTED(common_news_read_certificate, _cert_control::create_cert(
    "Common News Read",
    common_news_read_private_key,
    root_user_cert,
    *private_info.root_private_key_));
  save_certificate(common_news_read_certificate_, common_news_read_certificate);

  GET_EXPECTED(common_news_write_certificate, _cert_control::create_cert(
    "Common News Write",
    *private_info.common_news_write_private_key_,
    root_user_cert,
    *private_info.root_private_key_));
  save_certificate(common_news_write_certificate_, common_news_write_certificate);

  GET_EXPECTED(common_news_admin_certificate, _cert_control::create_cert(
    "Common News Admin",
    *private_info.common_news_admin_private_key_,
    root_user_cert,
    *private_info.root_private_key_));
  save_certificate(common_news_admin_certificate_, common_news_admin_certificate);

  //
  GET_EXPECTED(common_storage_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  save_private_key(common_storage_private_key_, common_storage_private_key);
  GET_EXPECTED(common_storage_certificate, _cert_control::create_cert(
    "Common Storage",
    common_storage_private_key,
    root_user_cert,
    *private_info.root_private_key_));
  save_certificate(common_storage_certificate_, common_storage_certificate);

  return expected<void>();
}
