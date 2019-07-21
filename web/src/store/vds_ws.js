class vds_ws {

    constructor() {
        this.callback_ = new Map();
        this.subscription_ = new Map();
        this.id_ = 0;
        this.is_opened_ = false;
    }

    async connect() {
        this.ws_ = new WebSocket('ws://localhost:8050/api/ws');

        var promise = new Promise((resolve, reject) => {
            this.callback_.set(0, { resolve, reject });
        });

        this.ws_.onopen = event => {
            const handler = this.callback_.get(0);
            this.callback_.delete(0);
            handler.resolve();
        };

        this.ws_.onclose = event => {
            this.callback_.forEach(function (item) {
                item.reject("Соединение прервано");
            });
            this.callback_.clear();
            this.ws_ = null;
        };

        this.ws_.onmessage = event => {
            const msg = JSON.parse(event.data);
            const id = +msg.id;
            const handler = this.callback_.get(id);
            if(handler){
                this.callback_.delete(id);
                if(msg.result){
                    handler.resolve(msg.result);
                }
                else {
                    handler.reject(msg.error);
                }
            }
            else {
                const subscription = this.subscription_.get(id);
                if(subscription){
                    subscription(msg.result);
                }
            }
        };

        this.ws_.onerror = event => {
            this.callback_.forEach(function (item) {
                item.reject("Соединение прервано");
            });
            this.callback_.clear();
        };

        return await promise;
    }

    async invoke(method, message) {
        if (null == this.ws_) {
            await this.connect();
        }

        this.id_++;
        const id = this.id_;
        let promise = new Promise((resolve, reject) => {
            this.callback_.set(id, { resolve, reject });
        });

        this.ws_.send(JSON.stringify({
            id: id,
            invoke: method,
            params: message
        }));

        return await promise;
    }

    async subscribe(method, params, callback){
        if (null == this.ws_) {
            await this.connect();
        }

        this.id_++;
        const id = this.id_;
        this.subscription_.set(id, callback);

        return await this.invoke("subscribe", [ id, method, params ]);
    }

}

export default vds_ws;