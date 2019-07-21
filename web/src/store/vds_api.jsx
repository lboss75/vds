import vds_ws from './vds_ws';
import { user_credentials_to_key, decrypt_private_key, parse_public_key, hash_sha256, public_key_to_der, base64 } from './vds_crypto';

const vdsApiConnectType = 'VDS_API_CONNECT';
const vdsApiConnectedType = 'VDS_API_CONNECTED';
const vdsApiMessageType = 'VDS_API_MESSAGE';
const vdsApiClosedType = 'VDS_API_CLOSED';
const vdsApiErrorType = 'VDS_API_ERROR';
const vdsApiLoginType = 'VDS_API_LOGIN';
const vdsApiLoginedType = 'VDS_API_LOGINED';
const vdsApiLoginErrorType = 'VDS_API_LOGIN_ERROR';
const vdsApiPersonalMessageType = 'VDS_API_PERSONAL_MESSAGE';

const initialState =
{
  vdsApiState: 'offline',
  vdsApiLoginError: '',
  vdsApiWebSocket: null
};

export const actionCreators = {
  login: (email, password) => async(dispatch, getState) => {
    dispatch({ type: vdsApiLoginType });
    try{
      const ws = new vds_ws();
      const keys = await ws.invoke('login', [ user_credentials_to_key(email, password) ]);
      const private_key = decrypt_private_key(keys.private_key, password); 
      const public_key = parse_public_key(keys.public_key);

      await ws.subscribe('channel', base64(hash_sha256(public_key_to_der(public_key))), function(message){
        dispatch({ type: vdsApiPersonalMessageType, message });
      });

      dispatch({ type: vdsApiLoginedType, ws: ws, keys: { public_key: public_key, private_key: private_key } });

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
      break;
    }
  }

  return state;
};
