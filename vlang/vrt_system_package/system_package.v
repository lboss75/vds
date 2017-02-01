namespace system {
  
  interface stream {
  
    read(buffer:byte[], offset:int=0):int;
    
    write(buffer:byte[], size:int);
  }
  
  interface text_stream {
    read(buffer:char[], offset:int=0):int;
    
    write(buffer:char[], size:int);
  
    read_line():string
    {
      var result = new string_builder();
      var buffer = new char[1];
      for(;;) {
        try {
          var readed = this.read(buffer:buffer);
          assert(readed == 1);
        }
        catch(end_of_file ex) {
          return result.to_string();
        }
        
        if(buffer[0] == 13) {
          return result.to_string();
        }
        
        result.add(symbol:buffer[0]);
      }
    }
  }
}