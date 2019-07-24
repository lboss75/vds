import React, { Component } from 'react'
import { Switch, Route } from 'react-router';
import Home from './components/Home';
import NavBar from './components/NavBar';
import Products from './components/Products';
import Contacts from './components/Contacts';
import VDS from './components/VDS';
import LoginPage from './components/LoginPage';
import Channels from './components/Channels';
import Channel from './components/Channel';

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
          <Route exact path='/apps' component={Channels} />
          <Route exact path='/app/:id' component={Channel} />
        </Switch>
      </NavBar>
    );
  }
}

export default App;
