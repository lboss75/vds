const vdsApiConnectType = 'VDS_API_CONNECT';

const initialState = 
{
    vdsApiState: 'offline'
};

export const actionCreators = {
  connect: () => ({ type: vdsApiConnectType }),
};

export const reducer = (state, action) => {
  state = state || initialState;

  switch(action.type)
  {
    case vdsApiConnectType:
    {
        return { ...state, vdsApiState: 'connecting' };
    }

  }

  return state;
};
