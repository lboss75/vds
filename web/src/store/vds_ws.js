class vds_ws {

    constructor() {
        this.callback_ = new Map();
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
            handler.promise();
        };

        this.ws_.onclose = event => {
            this.ws_ = null;
        };

        this.ws_.onmessage = event => {
            const msg = event.data;
            const id = msg.id;
            const handler = this.callback_.get(id);
            this.callback_.delete(id);
            handler.promise(msg.result);
        };

        this.ws_.onerror = event => {
            this.callback_.forEach(function (item) {
                item.reject("Соединение прервано");
            });
            this.callback_.clear();
        };

        return await promise;
    }

    async invoke(message) {
        if (null == this.ws_) {
            await this.connect();
        }

        this.id_++;
        const id = this.id_;
        let promise = new Promise((result, reject) => {
            this.callback_.set(id, { result, reject });
        });

        this.ws_.send(JSON.stringify({
            id: id,
            type: 'call',
            params: message
        }));

        return await promise;
    }

}

export default vds_ws;