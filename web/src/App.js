import React, { Component } from 'react'
import { Route } from 'react-router';
import Home from './components/Home';
import NavBar from './components/NavBar';
import Products from './components/Products';
import Contacts from './components/Contacts';

class App extends Component {
  render() {
    return (
      <NavBar>
        <Route exact path='/' component={Home} />
        <Route exact path='/products' component={Products} />
        <Route exact path='/contacts' component={Contacts} />
      </NavBar>
    );
  }
}

export default App;
