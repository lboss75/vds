import { 
    decode64,
    decrypt_by_private_key,
    decrypt_by_aes_256_cbc,
    buffer_pop_data,
    buffer_get_string,
    base64,
    hash_sha256,
    crypt_by_aes_256_cbc,
    create_buffer,
    from_hex } from './vds_crypto';
import pako from 'pako';

class vds_ws {

    constructor() {
        this.callback_ = new Map();
        this.subscription_ = new Map();
        this.read_keys_ = new Map();
        this.write_keys_ = new Map();

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

    add_read_key(id, key){
        this.read_keys_.set(id, key);
    }

    add_write_key(id, key){
        this.write_keys_.set(id, key);
    }

    decrypt(message) {
        const read_keys = this.read_keys_.get(message.read_id);
        const key_data = decrypt_by_private_key(read_keys.private_key, decode64(message.crypted_key));
        const data = decrypt_by_aes_256_cbc(key_data, decode64(message.crypted_data));

        const message_id = data.getByte();
        switch(message_id){
            //channel_create_transaction
            case 110:{
                const channel_id = base64(buffer_pop_data(data));
                const channel_type = buffer_get_string(data);
                const name = buffer_get_string(data);
                const read_public_key = buffer_pop_data(data);
                const read_private_key = buffer_pop_data(data);
                const write_public_key = buffer_pop_data(data);
                const write_private_key = buffer_pop_data(data);

                if(0 != data.length()){
                    throw "Invalid message";
                }

                return {
                    type: 'channel_create',
                    channel_id: channel_id,
                    channel_type: channel_type,
                    channel_name: name
                };
            }
        }

        throw "Invalid message";
    }

    async save_block(data){
        const buffer = create_buffer(data);
        const key_data = hash_sha256(buffer);
        const iv_data = from_hex('a5bb9fcec2e44b91a8c9594462559024');

        const key_data2 = hash_sha256(crypt_by_aes_256_cbc(key_data, iv_data, buffer.data));
        const zipped = pako.deflate(buffer.data);
        const crypted_data = crypt_by_aes_256_cbc(key_data2, iv_data, zipped);
        const result = await this.invoke('upload', [base64(crypted_data)]);
        return result;
    }

    async save_file(channel, name, chunks){
        const result = await this.invoke('upload', [channel, name, chunks]);
        return result;
    }
}

export default vds_ws;