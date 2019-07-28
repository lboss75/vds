import React, { Component } from 'react'
import { Switch, Route } from 'react-router';
import Home from './pages/public/Home';
import NavBar from './components/NavBar';
import Products from './pages/public/Products';
import Contacts from './pages/public/Contacts';
import VDS from './pages/public/VDS';
import LoginPage from './pages/user/LoginPage';
import Channels from './pages/user/Channels';
import Channel from './pages/user/Channel';

class App extends Component {
  render() {
    return (
      <NavBar>
        <Switch>
          <Route exact path='/' component={Home} />
          <Route exact path='/products' component={Products} />
          <Route exact path='/vds' component={VDS} />
          <Route exact path='/contacts' component={Contacts} />
          <Route exact path='/login' component={LoginPage} />
          <Route exact path='/app' component={Channels} />
          <Route exact path='/app/:id' component={Channel} />
        </Switch>
      </NavBar>
    );
  }
}

export default App;
