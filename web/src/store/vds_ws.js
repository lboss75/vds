class vds_ws {

    constructor(client){
        this.callback_ = new Map(); 
        this.id_ = 0;
        this.client_ = client;
    }

    connect() {
        this.ws_ = new WebSocket('ws://localhost:8050/api/ws');
    
        webSocket.onopen = event => {
          this.client_.change_vds_state('opened');
        };
    
        webSocket.onmessage = event => {
            const msg = event.data;
            const id = msg.id;
            const handler = this.callback_.get(id);
            this.callback_.delete(id);
            handler.promise(msg.result);
        };
    
        webSocket.onerror = event => {
            this.callback_.forEach(function(item){
                item.reject(event.message);
            });
            this.callback_.clear();
            this.client_.change_vds_state('failed', event.message);
        };
    
        webSocket.onclose = event => {
            this.client_.change_vds_state('closed');
        };

    }

    async invoke(message) {
        this.id_++;
        const id = this.id_;
        let promise = new Promise((result, reject)=>
        {
            this.callback_.set(id, {result, reject});
        });

        this.ws_.send(JSON.stringify({
            id: id,
            type: 'call',
            params: message
        }));

        return await promise;
    }

}