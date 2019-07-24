import React from 'react';
import { connect } from 'react-redux';
import PropTypes from 'prop-types';
import { bindActionCreators } from 'redux';
import { actionCreators } from "../store/vds_api";
import { withStyles } from '@material-ui/core/styles';
import AppList from './AppList';

const styles = theme => ({
});

class Channels extends React.Component {
    constructor(props) {
      super(props);
      this.body = props.children;
    }

    render() {
        const { classes, theme } = this.props;
        return (
            <div>
            <h1>Приложения</h1>
            <p>Доступны следующие приложения:</p>
            <AppList />
          </div>
        
        );
    }
}

Channels.propTypes = {
    classes: PropTypes.object.isRequired,
    theme: PropTypes.object.isRequired,
  };
  
  export default connect(
    state => state.vdsApi,
    dispatch => bindActionCreators(actionCreators, dispatch)
  )(withStyles(styles, { withTheme: true })(Channels));
  