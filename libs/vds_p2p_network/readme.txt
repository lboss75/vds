transport:



crypto_tunnel:
 sender:                            client:
    command_id ::CertCain
                                       gen output_key, command_id::SendKey
                                       command_id::CertRequest

    command_id::SendKey,input_key_

 client:
    enum class command_id : uint8_t {
      Data = 0,
      CertRequest = 1,//login, hash(password)
      CertRequestFailed = 2,//
      CertCain = 3, //size + certificate chain
      SendKey = 4, //size + certificate chain
      Crypted = 5
    };
