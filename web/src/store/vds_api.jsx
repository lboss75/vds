import vds_ws from './vds_ws';
import { user_credentials_to_key, decrypt_private_key, parse_public_key, public_key_fingerprint, base64 } from './vds_crypto';
import { push } from 'react-router-redux';

const vdsApiConnectType = 'VDS_API_CONNECT';
const vdsApiConnectedType = 'VDS_API_CONNECTED';
const vdsApiMessageType = 'VDS_API_MESSAGE';
const vdsApiClosedType = 'VDS_API_CLOSED';
const vdsApiErrorType = 'VDS_API_ERROR';
const vdsApiLoginType = 'VDS_API_LOGIN';
const vdsApiLoginedType = 'VDS_API_LOGINED';
const vdsApiLoginErrorType = 'VDS_API_LOGIN_ERROR';
const vdsApiPersonalMessageType = 'VDS_API_PERSONAL_MESSAGE';
const vdsApiChannelMessageType = 'VDS_API_CHANNEL_MESSAGE';

const initialState =
{
  vdsApiState: 'offline',
  vdsApiLoginError: '',
  vdsApiWebSocket: null,
  vdsApiChannels: new Map(),
};

export const actionCreators = {
  login: (email, password) => async(dispatch, getState) => {
    dispatch({ type: vdsApiLoginType });
    try{
      const ws = new vds_ws();
      const keys = await ws.invoke('login', [ user_credentials_to_key(email, password) ]);
      const private_key = decrypt_private_key(keys.private_key, password); 
      const public_key = parse_public_key(keys.public_key);
      const public_key_id = base64(public_key_fingerprint(public_key));

      ws.add_read_key(public_key_id, { public_key, private_key });
      ws.add_write_key(public_key_id, { public_key, private_key });

      await ws.subscribe('channel', public_key_id, function(message){       
        message.forEach(async message => {
          var msg = getState().vdsApi.vdsApiWebSocket.decrypt(message);
          switch(msg.type){
            case 'channel_create': {
              await ws.subscribe('channel', msg.channel_id, function(items){
                items.forEach(item => {
                  var item_msg = getState().vdsApi.vdsApiWebSocket.decrypt(item);
                  dispatch({ type: vdsApiChannelMessageType, channel: msg.channel_id, message: item_msg });
                });
              });

              dispatch({ type: vdsApiPersonalMessageType, message: msg });
              break;
            }
          }  
        });
      });

      dispatch({ type: vdsApiLoginedType, ws: ws, keys: { public_key: public_key, private_key: private_key } })
    }
    catch(ex){
      dispatch({ type: vdsApiLoginErrorType, error: ex });
    }
  }
};

export const reducer = (state, action) => {
  state = state || initialState;

  switch (action.type) {
    case vdsApiConnectType:
      {
        return { ...state, vdsApiState: 'connecting' };
      }
    case vdsApiConnectedType:
      {
        return { ...state, vdsApiState: 'connected', vdsApiWebSocket: action.ws };
      }
    case vdsApiMessageType:
      {
        return { ...state, vdsApiState: 'message' };
      }
    case vdsApiClosedType:
      {
        return { ...state, vdsApiState: 'closed' };
      }
    case vdsApiLoginType:
        {
          return { ...state, vdsApiState: 'login', vdsApiLoginError: '' };
        }
    case vdsApiLoginedType:
      {
        return { ...state, vdsApiState: 'logined',  vdsApiWebSocket: action.ws, vdsApiKeys: action.keys };
      }
    case vdsApiLoginErrorType:
      {
        return { ...state, vdsApiState: 'failed',  vdsApiLoginError: action.error };
      }

    case vdsApiErrorType:
    {
      return { ...state, vdsApiState: 'error' };
    }

    case vdsApiPersonalMessageType:
    {
      var newChannels = new Map(state.vdsApiChannels);
      newChannels.set(action.message.channel_id, { ...action.message, messages: []});
      return { ...state, vdsApiChannels: newChannels };
    }

    case vdsApiChannelMessageType:
    {
      var channel = state.vdsApiChannels.get(action.channel_id);
      channel.messages.push(action.message);
      return { ...state, vdsApiChannels: newChannels };
    }
  }

  return state;
};
