import React, { Component } from 'react'
import { Route } from 'react-router';
import Home from './components/Home';
import NavBar from './components/NavBar';
import Products from './components/Products';
import Contacts from './components/Contacts';
import VDS from './components/VDS';
import LoginPage from './components/LoginPage';

class App extends Component {
  render() {
    return (
      <NavBar>
        <Route exact path='/' component={Home} />
        <Route exact path='/products' component={Products} />
        <Route exact path='/vds' component={VDS} />
        <Route exact path='/contacts' component={Contacts} />
        <Route exact path='/login' component={LoginPage} />
      </NavBar>
    );
  }
}

export default App;
