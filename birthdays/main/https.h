#pragma once

// ====================================================================
// INCLUDES
// ====================================================================

#include "common.h"

// ====================================================================
// DEFINES
// ====================================================================

#define HTTPS_PUB_CERT "-----BEGIN CERTIFICATE-----\nMIIRDTCCD/WgAwIBAgIQTrF2gmsgAD4KAAAAASfcRjANBgkqhkiG9w0BAQsFADBG\nMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExM\nQzETMBEGA1UEAxMKR1RTIENBIDFDMzAeFw0yMTEyMDgyMjE1NTBaFw0yMjAzMDIy\nMjE1NDlaMCIxIDAeBgNVBAMMFyouZ29vZ2xldXNlcmNvbnRlbnQuY29tMFkwEwYH\nKoZIzj0CAQYIKoZIzj0DAQcDQgAE8mnTfEI33SzNanU1qf2eRAimo9f1vDk3r7DH\nHH7a9b2IjeBobrfjUkr6WWXzFvV/g0a7hQiBvo/ggXBpyuaZyaOCDuQwgg7gMA4G\nA1UdDwEB/wQEAwIHgDATBgNVHSUEDDAKBggrBgEFBQcDATAMBgNVHRMBAf8EAjAA\nMB0GA1UdDgQWBBRAmV1z0XEKEC2N14S0nJKfAsAM+zAfBgNVHSMEGDAWgBSKdH+v\nhc3ulc09nNDiRhTzcTUdJzBqBggrBgEFBQcBAQReMFwwJwYIKwYBBQUHMAGGG2h0\ndHA6Ly9vY3NwLnBraS5nb29nL2d0czFjMzAxBggrBgEFBQcwAoYlaHR0cDovL3Br\naS5nb29nL3JlcG8vY2VydHMvZ3RzMWMzLmRlcjCCDJMGA1UdEQSCDIowggyGghcq\nLmdvb2dsZXVzZXJjb250ZW50LmNvbYIgY29tbW9uZGF0YXN0b3JhZ2UuZ29vZ2xl\nYXBpcy5jb22CIiouY29tbW9uZGF0YXN0b3JhZ2UuZ29vZ2xlYXBpcy5jb22CFnN0\nb3JhZ2UuZ29vZ2xlYXBpcy5jb22CGCouc3RvcmFnZS5nb29nbGVhcGlzLmNvbYIb\nc3RvcmFnZS5tdGxzLmdvb2dsZWFwaXMuY29tgiQqLmFwcHNwb3QuY29tLnN0b3Jh\nZ2UuZ29vZ2xlYXBpcy5jb22CICouY29udGVudC1zdG9yYWdlLmdvb2dsZWFwaXMu\nY29tgiMqLmNvbnRlbnQtc3RvcmFnZS1wMi5nb29nbGVhcGlzLmNvbYInKi5jb250\nZW50LXN0b3JhZ2UtdXBsb2FkLmdvb2dsZWFwaXMuY29tgikqLmNvbnRlbnQtc3Rv\ncmFnZS1kb3dubG9hZC5nb29nbGVhcGlzLmNvbYIfKi5zdG9yYWdlLXVwbG9hZC5n\nb29nbGVhcGlzLmNvbYIhKi5zdG9yYWdlLWRvd25sb2FkLmdvb2dsZWFwaXMuY29t\nggxibG9nc3BvdC5jb22CDiouYmxvZ3Nwb3QuY29tgg9icC5ibG9nc3BvdC5jb22C\nESouYnAuYmxvZ3Nwb3QuY29tghpkb3VibGVjbGlja3VzZXJjb250ZW50LmNvbYIc\nKi5kb3VibGVjbGlja3VzZXJjb250ZW50LmNvbYIJZ2dwaHQuY29tggsqLmdncGh0\nLmNvbYIPZ29vZ2xlZHJpdmUuY29tghEqLmdvb2dsZWRyaXZlLmNvbYIXKi5nb29n\nbGVzeW5kaWNhdGlvbi5jb22CISouc2FmZWZyYW1lLmdvb2dsZXN5bmRpY2F0aW9u\nLmNvbYIVZ29vZ2xldXNlcmNvbnRlbnQuY29tghB1c2VyY29udGVudC5nb29nghIq\nLnVzZXJjb250ZW50Lmdvb2eCGiouc2FuZGJveC51c2VyY29udGVudC5nb29ngiVt\nYW5pZmVzdC5jLm1haWwuZ29vZ2xldXNlcmNvbnRlbnQuY29tgiVtYW5pZmVzdC5s\naDMtZGEuZ29vZ2xldXNlcmNvbnRlbnQuY29tgiVtYW5pZmVzdC5saDMtZGIuZ29v\nZ2xldXNlcmNvbnRlbnQuY29tgiVtYW5pZmVzdC5saDMtZGMuZ29vZ2xldXNlcmNv\nbnRlbnQuY29tgiVtYW5pZmVzdC5saDMtZGQuZ29vZ2xldXNlcmNvbnRlbnQuY29t\ngiVtYW5pZmVzdC5saDMtZGUuZ29vZ2xldXNlcmNvbnRlbnQuY29tgiVtYW5pZmVz\ndC5saDMtZGYuZ29vZ2xldXNlcmNvbnRlbnQuY29tgiVtYW5pZmVzdC5saDMtZGcu\nZ29vZ2xldXNlcmNvbnRlbnQuY29tgiVtYW5pZmVzdC5saDMtZHouZ29vZ2xldXNl\ncmNvbnRlbnQuY29tgiJtYW5pZmVzdC5saDMuZ29vZ2xldXNlcmNvbnRlbnQuY29t\ngh5tYW5pZmVzdC5saDMucGhvdG9zLmdvb2dsZS5jb22CEmdvb2dsZXdlYmxpZ2h0\nLmNvbYIUKi5nb29nbGV3ZWJsaWdodC5jb22CDnRyYW5zbGF0ZS5nb29nghAqLnRy\nYW5zbGF0ZS5nb29ngiQqLmRldi5hbXA0bWFpbC5nb29nbGV1c2VyY29udGVudC5j\nb22CJSoucHJvZC5hbXA0bWFpbC5nb29nbGV1c2VyY29udGVudC5jb22CKyoucGxh\neWdyb3VuZC5hbXA0bWFpbC5nb29nbGV1c2VyY29udGVudC5jb22CNCoucGxheWdy\nb3VuZC1pbnRlcm5hbC5hbXA0bWFpbC5nb29nbGV1c2VyY29udGVudC5jb22CKyou\nYWlwbGF0Zm9ybS10cmFpbmluZy5nb29nbGV1c2VyY29udGVudC5jb22CPSouYXVk\naW9ib29rLWFkZGl0aW9uYWwtbWF0ZXJpYWwtc3RhZ2luZy5nb29nbGV1c2VyY29u\ndGVudC5jb22CNSouYXVkaW9ib29rLWFkZGl0aW9uYWwtbWF0ZXJpYWwuZ29vZ2xl\ndXNlcmNvbnRlbnQuY29tghwqLmFwcHMuZ29vZ2xldXNlcmNvbnRlbnQuY29tgh8q\nLnNhZmVudXAuZ29vZ2xldXNlcmNvbnRlbnQuY29tgh8qLnNhbmRib3guZ29vZ2xl\ndXNlcmNvbnRlbnQuY29tgiAqLmNvbXBvc2VyLmdvb2dsZXVzZXJjb250ZW50LmNv\nbYIoKi5jb21wb3Nlci1zdGFnaW5nLmdvb2dsZXVzZXJjb250ZW50LmNvbYIjKi5j\nb21wb3Nlci1xYS5nb29nbGV1c2VyY29udGVudC5jb22CJCouY29tcG9zZXItZGV2\nLmdvb2dsZXVzZXJjb250ZW50LmNvbYIgKi5kYXRhcGxleC5nb29nbGV1c2VyY29u\ndGVudC5jb22CKCouZGF0YXBsZXgtc3RhZ2luZy5nb29nbGV1c2VyY29udGVudC5j\nb22CJCouZGF0YXBsZXgtZGV2Lmdvb2dsZXVzZXJjb250ZW50LmNvbYIgKi5kYXRh\ncHJvYy5nb29nbGV1c2VyY29udGVudC5jb22CLiouZGF0YXByb2MtaW1hZ2Utc3Rh\nZ2luZy5nb29nbGV1c2VyY29udGVudC5jb22CKCouZGF0YXByb2Mtc3RhZ2luZy5n\nb29nbGV1c2VyY29udGVudC5jb22CJSouZGF0YXByb2MtdGVzdC5nb29nbGV1c2Vy\nY29udGVudC5jb22CIiouZGF0YWZ1c2lvbi5nb29nbGV1c2VyY29udGVudC5jb22C\nKiouZGF0YWZ1c2lvbi1zdGFnaW5nLmdvb2dsZXVzZXJjb250ZW50LmNvbYImKi5k\nYXRhZnVzaW9uLWRldi5nb29nbGV1c2VyY29udGVudC5jb22CJiouZGF0YWZ1c2lv\nbi1hcGkuZ29vZ2xldXNlcmNvbnRlbnQuY29tgi4qLmRhdGFmdXNpb24tYXBpLXN0\nYWdpbmcuZ29vZ2xldXNlcmNvbnRlbnQuY29tgioqLmRhdGFmdXNpb24tYXBpLWRl\ndi5nb29nbGV1c2VyY29udGVudC5jb22CGyouZ3NjLmdvb2dsZXVzZXJjb250ZW50\nLmNvbYIbKi5nY2MuZ29vZ2xldXNlcmNvbnRlbnQuY29tghsqLnR1Zi5nb29nbGV1\nc2VyY29udGVudC5jb22CJCoudHVmLWF1dG9wdXNoLmdvb2dsZXVzZXJjb250ZW50\nLmNvbYIfKi50dWYtZGV2Lmdvb2dsZXVzZXJjb250ZW50LmNvbYIjKi50dWYtc3Rh\nZ2luZy5nb29nbGV1c2VyY29udGVudC5jb22CJyouZnVjaHNpYS11cGRhdGVzLmdv\nb2dsZXVzZXJjb250ZW50LmNvbYIwKi5mdWNoc2lhLXVwZGF0ZXMtYXV0b3B1c2gu\nZ29vZ2xldXNlcmNvbnRlbnQuY29tgjUqLmZ1Y2hzaWEtdXBkYXRlcy1hdXRvcHVz\naC1xdWFsLmdvb2dsZXVzZXJjb250ZW50LmNvbYIrKi5mdWNoc2lhLXVwZGF0ZXMt\nZGV2Lmdvb2dsZXVzZXJjb250ZW50LmNvbYIvKi5mdWNoc2lhLXVwZGF0ZXMtc3Rh\nZ2luZy5nb29nbGV1c2VyY29udGVudC5jb22CISoubm90ZWJvb2tzLmdvb2dsZXVz\nZXJjb250ZW50LmNvbYIhKi5waXBlbGluZXMuZ29vZ2xldXNlcmNvbnRlbnQuY29t\ngiMqLnRlbnNvcmJvYXJkLmdvb2dsZXVzZXJjb250ZW50LmNvbYIsKi50ZW5zb3Ji\nb2FyZC1hdXRvcHVzaC5nb29nbGV1c2VyY29udGVudC5jb22CJyoudGVuc29yYm9h\ncmQtZGV2Lmdvb2dsZXVzZXJjb250ZW50LmNvbYIrKi50ZW5zb3Jib2FyZC1zdGFn\naW5nLmdvb2dsZXVzZXJjb250ZW50LmNvbYIoKi50ZW5zb3Jib2FyZC10ZXN0Lmdv\nb2dsZXVzZXJjb250ZW50LmNvbYIfKi5rZXJuZWxzLmdvb2dsZXVzZXJjb250ZW50\nLmNvbYInKi5rZXJuZWxzLXN0YWdpbmcuZ29vZ2xldXNlcmNvbnRlbnQuY29tgiQq\nLmtlcm5lbHMtdGVzdC5nb29nbGV1c2VyY29udGVudC5jb20wIQYDVR0gBBowGDAI\nBgZngQwBAgEwDAYKKwYBBAHWeQIFAzA8BgNVHR8ENTAzMDGgL6AthitodHRwOi8v\nY3Jscy5wa2kuZ29vZy9ndHMxYzMvUXFGeGJpOU00OGMuY3JsMIIBBQYKKwYBBAHW\neQIEAgSB9gSB8wDxAHYAw2X5s2VPMoPHnamOk9dBj1ure+MlLJjh0vBLuetCfSMA\nAAF9nFUChwAABAMARzBFAiEA5sDBjIQ0/BJzgKjLLHHQysmgYIW/iRKLwjnce4+o\nJfECIEdKRkwQjVI1PNhzunhxNkcKITLAa/D8cBgEMMNqm0H1AHcARqVV63X6kSAw\ntaKJafTzfREsQXS+/Um4havy/HD+bUcAAAF9nFUAogAABAMASDBGAiEAmbT9pZzc\nfciv7Qhm/ghJ/EqAHbWSzJXfV5seX8YcsvkCIQCKsEsJ49Ii1PPswGYPhP0VeMD6\nS+D8v4PtQ8V/FRu4IzANBgkqhkiG9w0BAQsFAAOCAQEANtG/4cOB/GeX2r7p8dYb\n/b0H1YayuRfZt+ZvsoYKZm/z/4X6Yc0eqwREy4rQCz1hAMDbV72cCXLYPAVfKqwk\nxnV4A8UnAUwaIOGlO6qAXpv0tnoS/wTSl3a2EH2TTeUnkJNb3/2c99DpcZ9YLMeM\nRF7muvHUIBL8fSmwhdPvbq3UT20fDT8hdUOk8N5miQOPZjXt9VHL/ZYTpt5HER/q\nYq2ORTljmcGfKDa9FFSUyK2P179ETTGxRwOwDtvdHfH20lSr1kvMZyVMw8gF+39Q\nTgdr1Ne6SDx3BXN/RYn7V8GsJ+XI+ROm/SnXSA6NdUunhGYxDo5J7P0zu0fHYQ36\nYw==\n-----END CERTIFICATE-----\n-----BEGIN CERTIFICATE-----\nMIIFljCCA36gAwIBAgINAgO8U1lrNMcY9QFQZjANBgkqhkiG9w0BAQsFADBHMQsw\nCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\nMBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMjAwODEzMDAwMDQyWhcNMjcwOTMwMDAw\nMDQyWjBGMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\nY2VzIExMQzETMBEGA1UEAxMKR1RTIENBIDFDMzCCASIwDQYJKoZIhvcNAQEBBQAD\nggEPADCCAQoCggEBAPWI3+dijB43+DdCkH9sh9D7ZYIl/ejLa6T/belaI+KZ9hzp\nkgOZE3wJCor6QtZeViSqejOEH9Hpabu5dOxXTGZok3c3VVP+ORBNtzS7XyV3NzsX\nlOo85Z3VvMO0Q+sup0fvsEQRY9i0QYXdQTBIkxu/t/bgRQIh4JZCF8/ZK2VWNAcm\nBA2o/X3KLu/qSHw3TT8An4Pf73WELnlXXPxXbhqW//yMmqaZviXZf5YsBvcRKgKA\ngOtjGDxQSYflispfGStZloEAoPtR28p3CwvJlk/vcEnHXG0g/Zm0tOLKLnf9LdwL\ntmsTDIwZKxeWmLnwi/agJ7u2441Rj72ux5uxiZ0CAwEAAaOCAYAwggF8MA4GA1Ud\nDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwEgYDVR0T\nAQH/BAgwBgEB/wIBADAdBgNVHQ4EFgQUinR/r4XN7pXNPZzQ4kYU83E1HScwHwYD\nVR0jBBgwFoAU5K8rJnEaK0gnhS9SZizv8IkTcT4waAYIKwYBBQUHAQEEXDBaMCYG\nCCsGAQUFBzABhhpodHRwOi8vb2NzcC5wa2kuZ29vZy9ndHNyMTAwBggrBgEFBQcw\nAoYkaHR0cDovL3BraS5nb29nL3JlcG8vY2VydHMvZ3RzcjEuZGVyMDQGA1UdHwQt\nMCswKaAnoCWGI2h0dHA6Ly9jcmwucGtpLmdvb2cvZ3RzcjEvZ3RzcjEuY3JsMFcG\nA1UdIARQME4wOAYKKwYBBAHWeQIFAzAqMCgGCCsGAQUFBwIBFhxodHRwczovL3Br\naS5nb29nL3JlcG9zaXRvcnkvMAgGBmeBDAECATAIBgZngQwBAgIwDQYJKoZIhvcN\nAQELBQADggIBAIl9rCBcDDy+mqhXlRu0rvqrpXJxtDaV/d9AEQNMwkYUuxQkq/BQ\ncSLbrcRuf8/xam/IgxvYzolfh2yHuKkMo5uhYpSTld9brmYZCwKWnvy15xBpPnrL\nRklfRuFBsdeYTWU0AIAaP0+fbH9JAIFTQaSSIYKCGvGjRFsqUBITTcFTNvNCCK9U\n+o53UxtkOCcXCb1YyRt8OS1b887U7ZfbFAO/CVMkH8IMBHmYJvJh8VNS/UKMG2Yr\nPxWhu//2m+OBmgEGcYk1KCTd4b3rGS3hSMs9WYNRtHTGnXzGsYZbr8w0xNPM1IER\nlQCh9BIiAfq0g3GvjLeMcySsN1PCAJA/Ef5c7TaUEDu9Ka7ixzpiO2xj2YC/WXGs\nYye5TBeg2vZzFb8q3o/zpWwygTMD0IZRcZk0upONXbVRWPeyk+gB9lm+cZv9TSjO\nz23HFtz30dZGm6fKa+l3D/2gthsjgx0QGtkJAITgRNOidSOzNIb2ILCkXhAd4FJG\nAJ2xDx8hcFH1mt0G/FX0Kw4zd8NLQsLxdxP8c4CU6x+7Nz/OAipmsHMdMqUybDKw\njuDEI/9bfU1lcKwrmz3O2+BtjjKAvpafkmO8l7tdufThcV4q5O8DIrGKZTqPwJNl\n1IXNDw9bg1kWRxYtnCQ6yICmJhSFm/Y3m6xv+cXDBlHz4n/FsRC6UfTd\n-----END CERTIFICATE-----\n-----BEGIN CERTIFICATE-----\nMIIFWjCCA0KgAwIBAgIQbkepxUtHDA3sM9CJuRz04TANBgkqhkiG9w0BAQwFADBH\nMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExM\nQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIy\nMDAwMDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNl\ncnZpY2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEB\nAQUAA4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaM\nf/vo27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vX\nmX7wCl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7\nzUjwTcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0P\nfyblqAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtc\nvfaHszVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4\nZor8Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUsp\nzBmkMiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOO\nRc92wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYW\nk70paDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+\nDVrNVjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgF\nlQIDAQABo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNV\nHQ4EFgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBADiW\nCu49tJYeX++dnAsznyvgyv3SjgofQXSlfKqE1OXyHuY3UjKcC9FhHb8owbZEKTV1\nd5iyfNm9dKyKaOOpMQkpAWBz40d8U6iQSifvS9efk+eCNs6aaAyC58/UEBZvXw6Z\nXPYfcX3v73svfuo21pdwCxXu11xWajOl40k4DLh9+42FpLFZXvRq4d2h9mREruZR\ngyFmxhE+885H7pwoHyXa/6xmld01D1zvICxi/ZG6qcz8WpyTgYMpl0p8WnK0OdC3\nd8t5/Wk6kjftbjhlRn7pYL15iJdfOBL07q9bgsiG1eGZbYwE8na6SfZu6W0eX6Dv\nJ4J2QPim01hcDyxC2kLGe4g0x8HYRZvBPsVhHdljUEn2NIVq4BjFbkerQUIpm/Zg\nDdIx02OYI5NaAIFItO/Nis3Jz5nu2Z6qNuFoS3FJFDYoOj0dzpqPJeaAcWErtXvM\n+SUWgeExX6GjfhaknBZqlxi9dnKlC54dNuYvoS++cJEPqOba+MSSQGwlfnuzCdyy\nF62ARPBopY+Udf90WuioAnwMCeKpSwughQtiue+hMZL77/ZRBIls6Kl0obsXs7X9\nSQ98POyDGCBDTtWTurQ0sR8WNh8M5mQ5Fkzc4P4dyKliPUDqysU0ArSuiYgzNdws\nE3PYJ/HQcu51OyLemGhmW/HGY0dVHLqlCFF1pkgl\n-----END CERTIFICATE-----\n"

// ====================================================================
// TYPEDEFS
// ====================================================================

// ====================================================================
// GLOBAL VARIABLES
// ====================================================================

// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================

bool get_birthday_list();