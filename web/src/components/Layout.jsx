import React from 'react';
import NavBar from './NavBar';
import './Layout.css';

export default props => (
  <div class="layout">
    <NavBar />
    {props.children}
  </div>
);
