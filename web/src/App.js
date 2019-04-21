import React, { Component } from 'react'
import { Route } from 'react-router';
import Layout from './components/Layout';
import Home from './components/Home';

class App extends Component {
  render() {
    return (
      <Layout>
        <Route exact path='/' component={Home} />
      </Layout>
    );
  }
}

export default App;
