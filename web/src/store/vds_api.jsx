const vdsApiConnectType = 'VDS_API_CONNECT';
const vdsApiConnectedType = 'VDS_API_CONNECTED';
const vdsApiMessageType = 'VDS_API_MESSAGE';
const vdsApiClosedType = 'VDS_API_CLOSED';
const vdsApiErrorType = 'VDS_API_ERROR';
const vdsApiLoginType = 'VDS_API_LOGIN';

const initialState =
{
  vdsApiState: 'offline',
  vdsApiWebSocket: null
};

export const actionCreators = {
  connect: () => async (dispatch, getState) => {
    const webSocket = new WebSocket('ws://localhost:8050/api/ws');
    dispatch({ type: vdsApiConnectType, ws: webSocket });

    webSocket.onopen = event => {
      dispatch({ type: vdsApiConnectedType });
    };

    webSocket.onmessage = event => {
      dispatch({ type: vdsApiMessageType, message: event.data });
    };

    webSocket.onerror = event => {
      dispatch({ type: vdsApiErrorType, error: event.message });
    };

    webSocket.onclose = event => {
      dispatch({ type: vdsApiClosedType });
    };
  },
  login: (email, password) => async(dispatch, getState) => {
    dispatch({ type: vdsApiLoginType });
    const ws = getState().vdsApi.ws;
    ws.send(JSON.stringify({email, password}));
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
        return { ...state, vdsApiState: 'connected' };
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
        case vdsApiErrorType:
      {
        return { ...state, vdsApiState: 'error' };
      }
  }

  return state;
};
