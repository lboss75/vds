#include "stdafx.h"
#include "private/cert_control_p.h"
#include "cert_control.h"

char vds::cert_control::root_certificate_[1821] =
"MIIFTzCCAzegAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA4MDkwNzE1NTZaFw0xOTA4MDkwNzE1"
"NTZaMDoxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRkwFwYDVQQDDBB2YWRpbUBpdi1zb2Z0"
"LnJ1MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAktKnDkcwxjqe07vtVs0G2gjQMalc/ojE"
"NqmsH3j3EdAMGjZ4QacxFJWS/hZRWiYc0qMgh8sHwMoql6QNFPZsw4YD8fDHH4kDKdQymGZeR3XuQgO2"
"SLhl4qcNBbiWCfNcuzwtb2ltKQ1mwSYwv7QseKlyOKbh/5gUqkj/HoniBq4xKbg8jS4BNkJKLKPgc8J4"
"dHlbmoFtZl0vBMntj/K0rAVAF1+3adSkm3tEBnIdlpidxEn0Y42TG2gIWjydJFRg5hT8Hf+pxFA7cBPE"
"NdujbxZ2D/hmWMTaLe4wv36GQfnrRr31m1xqo3P7uuL9BmNavqfUwAgkMudk6XPaC1acEhrz6RvRecga"
"WZ8/3+X4xuncoKonuyREb+Ehs1PYxYLodm3bY5lEoJUprFRts1MktsRTqvEIg1cggmL0fj08+jWJfHWc"
"JYvIq0BL1A47InRQXBTP9NYaP53ppM0VAsv8UEB1xhQlPKJf2LQXWR4mzgCMrxBgHMyP81dFkf6ohk0U"
"iitS602gTgcPEz9pXiUgCCN0n2Mbq4zs121P4yHrosuxyWC2Z0bQW49l+PxhEhmzKCA5NINbE1a2lwIm"
"CxsTzEfkvQ1BF8/xt7tCHnIj3Spa1vEaxlM/9uJeyC5eNWf4ELvYBFXiCfyzVQ+2Q1ArNwy+cEyWF5bY"
"NbO/7BebIA0CAwEAAaNgMF4wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwEQYJYIZIAYb4"
"QgEBBAQDAgIEMCgGCWCGSAGG+EIBDQQbFhlleGFtcGxlIGNvbW1lbnQgZXh0ZW5zaW9uMA0GCSqGSIb3"
"DQEBCwUAA4ICAQAhBnR0Jggq4Pye3j6NBHsK8F1zQk1nU/CkzXQlx2V4J8fh4DYNcLXQGBgx/k44MmN7"
"qUrpZ6FGfH4/CQzvAlZiWKGeN5qsyMnZvL9O6y+aSbOrq23CKS1EIoAtYKWoQAfSng8JSz3JOUqkbqT2"
"phtJACKOBlrVPlkozELXMINcn2Z3mbJQEwVqduYHwHYkZ/bOCXuLdnBGdo5gc0yz2vvXzVzQUOUe7Jy2"
"bUm6t8oxpwoPIZf1pBwRALFcXjApLnVYuGlSiEk9afjD/A+fWg0c1HiAY5MUk2wqgRhRBEszun/+hX1b"
"sxanl41nAVR+F/MlSxwXMmdcgoCmlJCPLWoaxV/DLrNf8Kf/vr/JfPAe6/ZFHKotQ3kmvGpDvxACefgg"
"f8xijrtbgzL3N4f69Uk/n1ZH4C/Sh73nLb0gQO4Mk8H60/hn7fdAWoQVyGDgpfY8FXuRK7s4fZiVYXyd"
"EO8FomRurZpfN/XF33WyHdx86YzVrwrwVdObuU1okCYL3A0M6BzilH8r1bXAL+WKYOD+E0/isYZUKrvT"
"MWpMi5hts5WaNB1AygjlUWif59rRPQR91erehA5qPzbXnzTXxbI4eaWUk7ihD7YZHEHUxYuaEbMUT1Wj"
"aTD4RaqvyodmUBZpveu8o3+ReT6hopw6TvaqcX73i8OKkOrLU32wRst15w==";
char vds::cert_control::common_news_read_certificate_[1821] =
"MIIFTzCCAzegAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA4MDkwNzE1NTdaFw0xOTA4MDkwNzE1"
"NTdaMDoxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRkwFwYDVQQDDBBDb21tb24gTmV3cyBS"
"ZWFkMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA0cwvq7jKkIcCO4/nPMcJfNbrby0cSKxF"
"3gCs98puC9OHlFywicXeVY454ppOPBKQgrGtTjuEwqAgi/jUyNEhFokzen4CXt85DMhapVStfiLvdTBG"
"g7VVXjJjna1pw0dL97WmDjR2Dl+QCmpIHI8N+elRvI0uh4TYAOr9FGHhz60oMvfFbYglGk9n/+C3upDF"
"ZQazjdNmZsu8+pQVgXX3KINjOsp/AB2/llZQbW+852OXDUNC/TyRqo3159w0Q/Rg0svkfKa5SOao6ex6"
"800n/CCvkNW1lP0GZ4DEEXgs5kUv/rMk5t/FxOXz7ikhl4MqUXoPWD+UZzq9SliAfdN9S7yRyYJYgNkU"
"nd+4Dk6Ui+brt85/sTKwCulTJNpQgkxf0tjdEprDs0YNC/jDmilb3NqYWLRHtI7yRFVEG7yhtCbCtS4Y"
"A+0t/Ca2U/NggGyR5XTpG0OXH34J94o92R4iSWNTiJuMmpNuRuge3idGsAAaUGTytvfOJU7V3rIbfzRf"
"kd/bw4FJPOvEWHQJ60CLWIeaPbSsXog5NRT6f5cGGsHOwRQ1JjGqVm2fN0GMmTDNcS7mYzUZLUmESY1Y"
"8LeilNuGaR0ojAGmAQXqr9hZ+WZHgfutg599IG5b6W/x72ZTFs6xt8jClG7LiWqeMHYll1d/BDS5if9B"
"JoRDHezs+hMCAwEAAaNgMF4wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwEQYJYIZIAYb4"
"QgEBBAQDAgIEMCgGCWCGSAGG+EIBDQQbFhlleGFtcGxlIGNvbW1lbnQgZXh0ZW5zaW9uMA0GCSqGSIb3"
"DQEBCwUAA4ICAQA3zhsjoFIN6RGEym4f0pxdsNuozHxXEFG2IeBOl0QjPr1hiILzIVqFWBU1tDzsQdtd"
"gRyat9Ur42B0vKXUfZsqoeNU6sVE8LYhrVduYFXzGdEi/p9juNOud4PnwAP5Y7rcWzZekFxzwV9+X4Yx"
"IPEajhttu4HJmzXrDE1SG/RmdU/W/Jbn0J+G3w3XprlmSCPSMhG6WFYKOmSL3KD+NCHFRSvP6wwhMC2E"
"DVU8H3p1JNTp9+wnBF2s4MjytPWCSYEaNVt3EZmsyOPbZoe60dOxg8HdVIChimkENK0J0OJ4twnRQ2kY"
"l24uGMeZh4go1UtpuAca1evil/3hjtbp2xQqq7nwY+0DAiPKpH3jJnA85x5qE1bSmKkZI6vpttE0JfUE"
"dRonQ8aUNylW7J0dWdVK65pxM7ZmdvjZN6pw4sPdl9KgGiemwjoYaGuuiPHHMdZN5tBOGA/l50vburk/"
"rTGCqZk1eNcxepr0mhGT5uYVyXl99oPjhlnkdpI45Cn55+VyxU9FeK7LcPtgGYp+XRvx5wBaNgYZkJiB"
"sRl8AHOw1jT90csdZTHmyRPQDPe0aMk9+C5fkOzLYo8+8QZfbghchOlhqJSg9FVYI71pyBMZJcSORL6q"
"CXLti6RRB8WeUygODr7mQSpOMkz3Z1R2ogTQ0zLad7xF67f6D8YRHP/PMg==";
char vds::cert_control::common_news_read_private_key_[3137] =
"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"
"FWjrsNnowhaw";
char vds::cert_control::common_news_write_certificate_[1821] =
"MIIFUDCCAzigAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA4MDkwNzE1NTdaFw0xOTA4MDkwNzE1"
"NTdaMDsxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRowGAYDVQQDDBFDb21tb24gTmV3cyBX"
"cml0ZTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAMzOM6834uKF3Ys/e+LS3nVNXHRNjUE7"
"RuAZD7N+HhkQgkEGpD4Q4U06xlhLRFxB50hlYkJBj3g91IEcbW4n79lqJjZlEpM6ov4GS/6oAAkQDT+r"
"Az7FqlaJi8S7LiLj1gg3u7c9QoUGnT8fY0PMf9nM81gDq0OGORNn1YX11CjA28oPNfC6cSD89XRjVW3e"
"MiKriQnmv+PSZ/LD4BPE6NGGmm23Bf6cdNCJB2mEjzGCXbzvreHzK/ntky0Lw+87t0Lmisvj+WDXfokE"
"SUaA+OfHRAjVa/GLqcOXtQ4sqmBx++t8M5nTJZbJQx8B/6In5cvsZLnsA4LT52DnXBeBlGaDRgOulM56"
"d0bq1LwGbkvY+Eskd86hpZXLCBXpia5x+7XWMkxw5L798YsxCMzme5YVuFDHEzhM3UjlUx3asRUGehL1"
"5EeP4tGxdR5XtcnupywvztAmpNc/GgT5NFxankuJjD5wIvtqBBlKbFFXrQLaay9awn4JFpq/DDdVGB9r"
"1b5SUZBwTdRgDEQTzyPgfiTH91xpFAWC7lgzP65sdeklOO2Hm9wGYHUHxzu61N7aWESqAilSAKLmrIYO"
"IjrXQria9/R5OMu3NsSChgN+8KX3nx5L3I0+iVWCOzqycWL2BIyl0xVsYzLTr3TXXKkxrLL4iLd3tTAX"
"yXzT0Y7f+X0BAgMBAAGjYDBeMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMBEGCWCGSAGG"
"+EIBAQQEAwICBDAoBglghkgBhvhCAQ0EGxYZZXhhbXBsZSBjb21tZW50IGV4dGVuc2lvbjANBgkqhkiG"
"9w0BAQsFAAOCAgEAhO/oy+/r/jzKH8gLg945xzGK8q2PZVFQTbF/ylo9CGbkD8KeUYwZ/D52AAzvG+Xg"
"ED2+i+cXf8b+WM8bp+1r6HnSQzO229tdA8iWK4rk7h9Q3TRbSRUnqZSoodXv6wbOUkS6RzP1yr5CMBt6"
"OLQiZ0LvHLr4xspp3KISWIwhazfTd6QZBbZmvpckS10y04mMnuVWDyyrSUHtvU19BnMyYbGU+G0hT8IC"
"62HivqxkCv0oDGPD5+J52CL7TGQlZJPV0ExGxL504ORCoE7JQ1YDjNva3MCr4lHRa8K1fdiAVDYCjW/E"
"ee3VRCAj9e/5dYDfCZzyr8ANVWvJsZjhvAd1udTAOa8YrUuP6Gn3rQC1p4Fs5EUanzYxGnhHkKUH+ld0"
"8ybzzQuAO4cgnJ7GZYYh4fMbmInxWGhkjBSb6jREDjbf+Gxt5v31Rnv6wntSkkCkMDabd7jRworAwjWw"
"d6dssPB68BvEC6L2QajUXAhUuuoQU0KUOF4WoAQN0lDQIVoUSHiTCSGYqT4gxuHn8dTcMsoOBkUWZVVf"
"2yXLOxuDMorb2MdINs8E+dDrFpgyRzFqwwFRm6+sGJ5ia6V3nc7Qtaw8mrQKrvtHL7zTSUceh4u+MSj2"
"LWAk+H57CKlVhVFCBnIqzoR7XICWerWB5eAoGshMXYxWFRyXOAkPeI9xj8Y=";
char vds::cert_control::common_news_admin_certificate_[1821] =
"MIIFUDCCAzigAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA4MDkwNzE1NTdaFw0xOTA4MDkwNzE1"
"NTdaMDsxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRowGAYDVQQDDBFDb21tb24gTmV3cyBB"
"ZG1pbjCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAMQLHJA5sNVDznkwLh+L2By62fhzImEV"
"UH1G61yzkod6OOo6hKUGove42jbHp6GwF2Z0RX25hZzLZqqmNK7k/PHmk2HYL0TpY+/G0E5Pnz7kvKfS"
"umhFra/UOMv1j6SdKAltX5QfypVewzTK2R3NZYfPxlxrhFmFeUapmChCPhIDQg833yQRIb5UcXZKpA99"
"a0d9TP8m2qbt5koGpLbU7ra24eHvR/fxsug+69Leb+Gs58dW19BLXSXmU0ahXkqNc2c0kCWiDx4P8b88"
"uAbeByyA48ZdHhK2DH/gUIGtnokfjX5QHMmCxJgaTU3qlROruFHjoV1QkqBZL5LSy4Fgy1nhMYnApgPA"
"oF8M+dhKOE5CASP1NURVYK3qe9Dd/TLBjXg7BEzPXt/RcKMGBcBJiOZdDI23uUiZEuDTNYHhTxxJ/Mu1"
"6m9b/PnL+3RpUpk2vPtzhZDwBTW4Ner49fu6AwkBcjzy7cjZPmkAcaqLOdKAHX8AE1rDfgYpqZvDb6Nr"
"ht7+09ZvGTAWN2NBzONWdjTBZfIgjC0SNsheagClVsBr+FfKBPEaXIM2Bno67+E9XbL+typ0DuhtHfYP"
"nM22cunvO0Vqd3e2qsYUkQznK1vY2agb8s+ENw9znls9kbV38IAhzQVeSz0wwKM7yz8Kl+CEMmTJXjmh"
"a9XISlaKo6qJAgMBAAGjYDBeMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMBEGCWCGSAGG"
"+EIBAQQEAwICBDAoBglghkgBhvhCAQ0EGxYZZXhhbXBsZSBjb21tZW50IGV4dGVuc2lvbjANBgkqhkiG"
"9w0BAQsFAAOCAgEAAJhsE75wUoKec3oDmYV7I4O4DXK0SvqLfDvbFleHtv1FafH97cedjFiNKFG2y6DI"
"s+0v9vB9fsH3c0nvrvjXlLoA0O1h+b7Ge6ssTdFWWZwxmyWOxAOdzYBKgCxPmJVkippdlHVmpuS3b6U8"
"PpYESL9ALahAJDPCmeL3B3iZWtcy/Eb4Wuf1DY6gmE4l9VBVoJqQ/fFLt/q6f0W9BriC8NXYd2ScpWcR"
"CrYGJ4tIdLQG+o1MEYpNBKb9SPwfQV17WyU3rgS/03hir640l7RUaWgBeMn5URXSTQrlwiMz6nn1mxpx"
"74/UXS4kZYcyPVvm3B/gzXobDkAE741Wj723AcRcmaekionJUQM00BUtJBkNs8peKLxs/DfO4wtIjUSG"
"xge3fBh3F+LLRe5GZUccpSIYvzwNSPoNqTVN2W2fg4zjK3KlrjUyYiwxrvcLfnbbT6vjGdEq6pyMLDzr"
"ztJRcVrxWR8FspeUycYRPIT+H/iBRtAXrqYlqK6K29RVqKHZ1AqPk8eCOMc0xFPNYXifP8YbCa0X5FyB"
"ctRCbFNq7IJZrarqPGu1D3XBR40GS2byvgpqyuKm3Vlt2jeNd6lZ3hrjRwpZhrtGae6ZrBrcDH23MzTb"
"KPfGs7QRDuYA5S3g2aNPWVTr05hFDRGILRsPlBblxer04vbegUGli7oeEMY=";
char vds::cert_control::common_storage_certificate_[1821] =
"MIIFTTCCAzWgAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA4MDkwNzE1NTdaFw0xOTA4MDkwNzE1"
"NTdaMDgxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRcwFQYDVQQDDA5Db21tb24gU3RvcmFn"
"ZTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAMHUEHgijgEocG+ykCnuUgsvC/AqtxCbwa+e"
"BJTMfRaKF9ZUt4Ilf0E1NxkK9npQjGGA30L82UCS5gWAxUl25zJeoKEL+iB4x85/c0g2PreFsSftFvCD"
"RrC5NzgfAWRoZc6aVwrU8U6CJHoZAoWCGs3TMe4G0+YuIVl2IB5RHU9nQkF94xI7mwdGImkIeEsQWOQR"
"4EfrH9e5DN6vz/kKTxzVKHSN0yjFT2Ye5JEEjb5pVXqa8YgyYq+kDgYUnpACJs+AUu1/2eSnbn/9dm8Z"
"S/WOBfq+amGdnBoLqjH5qSl9QKqoPL3WIIwWRBKdvK7fYqW7vWoTvS2XLALmWaBXyecXfbyI29NIC2QG"
"Jt/PMczBRmHZ5YrfgdLOV+73wV2oMXucTiPfXxktzdPsc73vtyaf9ZEoi9oxgJTr9vEZfV+llqVvUXOU"
"O1dqhIfufYywgiO0uJskLNYQn/sUuSnHdtXbfZvb7xsZEqhemd4Gwhg0hwks9jcHZBHPdgmQGDcP+Zys"
"YxdI0ji6yo8X5LYonFoj45m0RB0Fu7kCfP1z3ihKxHziC0ntm1OkWfIbRunxqkYu81dBe8qeDlKONgMf"
"flDPBdypGwldxd7DfUhrGpN8qGLphfiQBEgTUml0kHTB7pkGYyIJfCkBdCGiTcgGAJLLdJ5WEtZof3KN"
"EW4vH7wbAgMBAAGjYDBeMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMBEGCWCGSAGG+EIB"
"AQQEAwICBDAoBglghkgBhvhCAQ0EGxYZZXhhbXBsZSBjb21tZW50IGV4dGVuc2lvbjANBgkqhkiG9w0B"
"AQsFAAOCAgEAYEuctPgAiG1WXn/W7ra2VYmfOk/hc3oCvPFdDooq8d7NvHcxHpGCmXzxQs5fCD7P8Nl5"
"VRVkcUCRDaKrS/E/1d3ji/vdV0ORqCWOx4a7ZqLbIC6dB1JSee5ALt19/+MQ3BqQ9E4epAdt9sYeaeBM"
"pIbgUlSXizXLXw3nLDWa8tI+YW/vx0gV64K3hI/h0LoR6gWPaC3opzv+UaXPXAESK8CI7HO4y952qJik"
"6KnC1PBu9VsuzTA+xrme3S6ThTbVS+VjtVdXRyiXeshWpiemI3VPe/WdOjMDMiW9WZT2DttYgWRr6kvv"
"w60egc2yURK8Eo8ILoL62kHD94bx2/ixBxyl3xH5knwEmJDDep7J5FWoOpcwij6ToqnWGEMuAeukNnGy"
"27KZdyrc50hR0OL7kOfWiZUJswj3O9FMqA31AxTk9qdY9yGKcZ9ju+CaRGikYFM+LADf54h6MwwcWvvZ"
"hhWkXpux6vGjNupltUTZ0tCF8oeQoUIYs16N8bUADi+4RzMgAS8GTcFnu/M3N+roiOYDejn4SKXKZcSE"
"wSMuqP/gsq03BqKFSetyqjTDDUBi8tLyLgenwzZ0ThmSRE6QjTlfZ4dxk0Q7oIr0odaNSt8RuVA18vqq"
"+EwBBybNFtclxpq+DOWZuygLRg9mFG/sNETrjF25QMldsJVdhDc25CY=";
char vds::cert_control::common_storage_private_key_[3137] =
"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"
"hb4H3i+tnBcf";

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
  this->root_private_key_ = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  this->common_news_write_private_key_ = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  this->common_news_admin_private_key_ = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
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
    private_info.root_private_key_);
  save_certificate(root_certificate_, root_user_cert);

  //
  const auto common_news_read_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  save_private_key(common_news_read_private_key_, common_news_read_private_key);
  const auto common_news_read_certificate = _cert_control::create_cert(
    "Common News Read",
    common_news_read_private_key,
    root_user_cert,
    private_info.root_private_key_);
  save_certificate(common_news_read_certificate_, common_news_read_certificate);

  const auto common_news_write_certificate = _cert_control::create_cert(
    "Common News Write",
    private_info.common_news_write_private_key_,
    root_user_cert,
    private_info.root_private_key_);
  save_certificate(common_news_write_certificate_, common_news_write_certificate);

  const auto common_news_admin_certificate = _cert_control::create_cert(
    "Common News Admin",
    private_info.common_news_admin_private_key_,
    root_user_cert,
    private_info.root_private_key_);
  save_certificate(common_news_admin_certificate_, common_news_admin_certificate);

  //
  const auto common_storage_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  save_private_key(common_storage_private_key_, common_storage_private_key);
  const auto common_storage_certificate = _cert_control::create_cert(
    "Common Storage",
    common_storage_private_key,
    root_user_cert,
    private_info.root_private_key_);
  save_certificate(common_storage_certificate_, common_storage_certificate);
}
