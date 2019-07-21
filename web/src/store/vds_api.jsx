import vds_ws from './vds_ws';
import user_credentials_to_key from './vds_crypto';

const vdsApiConnectType = 'VDS_API_CONNECT';
const vdsApiConnectedType = 'VDS_API_CONNECTED';
const vdsApiMessageType = 'VDS_API_MESSAGE';
const vdsApiClosedType = 'VDS_API_CLOSED';
const vdsApiErrorType = 'VDS_API_ERROR';
const vdsApiLoginType = 'VDS_API_LOGIN';
const vdsApiLoginedType = 'VDS_API_LOGINED';
const vdsApiLoginErrorType = 'VDS_API_LOGIN_ERROR';

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
      await ws.invoke({ method: 'login', key: user_credentials_to_key(email, password)});
      dispatch({ type: vdsApiLoginedType, ws: ws });
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
            return { ...state, vdsApiState: 'login' };
          }
          case vdsApiLoginedType:
            {
              return { ...state, vdsApiState: 'logined',  vdsApiWebSocket: action.ws };
            }
            case vdsApiLoginErrorType:
              {
                return { ...state, vdsApiState: 'failed',  vdsApiLoginError: action.error };
              }
            case vdsApiErrorType:
      {
        return { ...state, vdsApiState: 'error' };
      }
  }

  return state;
};
