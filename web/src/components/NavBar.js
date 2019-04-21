import React from 'react';
import PropTypes from 'prop-types';
import AppBar from '@material-ui/core/AppBar';
import Toolbar from '@material-ui/core/Toolbar';
import IconButton from '@material-ui/core/IconButton';
import Typography from '@material-ui/core/Typography';
import InputBase from '@material-ui/core/InputBase';
import { fade } from '@material-ui/core/styles/colorManipulator';
import { withStyles } from '@material-ui/core/styles';
import MenuIcon from '@material-ui/icons/Menu';
import SearchIcon from '@material-ui/icons/Search';
import HomeIcon from '@material-ui/icons/Home';
import AppsIcon from '@material-ui/icons/Apps';
import ContactsIcon from '@material-ui/icons/Contacts';
import MenuItem from '@material-ui/core/MenuItem';
import Menu from '@material-ui/core/Menu';
import AccountCircle from '@material-ui/icons/AccountCircle';
import Drawer from '@material-ui/core/Drawer';
import ChevronLeftIcon from '@material-ui/icons/ChevronLeft';
import ChevronRightIcon from '@material-ui/icons/ChevronRight';
import Divider from '@material-ui/core/Divider';
import ListItem from '@material-ui/core/ListItem';
import ListItemIcon from '@material-ui/core/ListItemIcon';
import ListItemText from '@material-ui/core/ListItemText';
import List from '@material-ui/core/List';
import InboxIcon from '@material-ui/icons/MoveToInbox';
import MailIcon from '@material-ui/icons/Mail';
import classNames from 'classnames';

const drawerWidth = 240;

const styles = theme => ({
 root: {
    width: '100%',
  },
  grow: {
    flexGrow: 1,
  },
  menuButton: {
    marginLeft: -12,
    marginRight: 20,
  },
  title: {
    display: 'none',
    [theme.breakpoints.up('sm')]: {
      display: 'block',
    },
  },
  search: {
    position: 'relative',
    borderRadius: theme.shape.borderRadius,
    backgroundColor: fade(theme.palette.common.white, 0.15),
    '&:hover': {
      backgroundColor: fade(theme.palette.common.white, 0.25),
    },
    marginLeft: 0,
    width: '100%',
    [theme.breakpoints.up('sm')]: {
      marginLeft: theme.spacing.unit,
      width: 'auto',
    },
  },
  searchIcon: {
    width: theme.spacing.unit * 9,
    height: '100%',
    position: 'absolute',
    pointerEvents: 'none',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
  },
  inputRoot: {
    color: 'inherit',
    width: '100%',
  },
  inputInput: {
    paddingTop: theme.spacing.unit,
    paddingRight: theme.spacing.unit,
    paddingBottom: theme.spacing.unit,
    paddingLeft: theme.spacing.unit * 10,
    transition: theme.transitions.create('width'),
    width: '100%',
    [theme.breakpoints.up('sm')]: {
      width: 120,
      '&:focus': {
        width: 400,
      },
    },
    },
 hide: {
     display: 'none',
 },
 drawer: {
     width: drawerWidth,
     flexShrink: 0,
 },
 drawerPaper: {
     width: drawerWidth,
 },
 drawerHeader: {
     display: 'flex',
     alignItems: 'center',
     padding: '0 8px',
     ...theme.mixins.toolbar,
     justifyContent: 'flex-end',
 },
 appBar: {
    transition: theme.transitions.create(['margin', 'width'], {
      easing: theme.transitions.easing.sharp,
      duration: theme.transitions.duration.leavingScreen,
    }),
  },
 appBarShift: {
    width: `calc(100% - ${drawerWidth}px)`,
    marginLeft: drawerWidth,
    transition: theme.transitions.create(['margin', 'width'], {
      easing: theme.transitions.easing.easeOut,
      duration: theme.transitions.duration.enteringScreen,
    }),
  },
  content: {
    flexGrow: 1,
    padding: theme.spacing.unit * 3,
    transition: theme.transitions.create('margin', {
      easing: theme.transitions.easing.sharp,
      duration: theme.transitions.duration.leavingScreen,
    }),
  },
 contentShift: {
    transition: theme.transitions.create('margin', {
      easing: theme.transitions.easing.easeOut,
      duration: theme.transitions.duration.enteringScreen,
    }),
    marginLeft: drawerWidth,
  },
});

class NavBar extends React.Component {
  constructor(props) {
    super(props);
    this.body = props.children;
  }

  state = {
    auth: true,
      anchorEl: null,
      openDrawer: false
  };

  handleChange = event => {
    this.setState({ auth: event.target.checked });
  };

  handleMenu = event => {
    this.setState({ anchorEl: event.currentTarget });
  };

  handleClose = () => {
    this.setState({ anchorEl: null });
    };

    handleDrawerOpen = () => {
        this.setState({ openDrawer: true });
    };

    handleDrawerClose = () => {
        this.setState({ openDrawer: false });
    };

  render() {
    const { classes, theme } = this.props;
    const { auth, anchorEl } = this.state;
      const open = Boolean(anchorEl);
      const { openDrawer } = this.state;

    return (
        <div className={classes.root}>
        <AppBar
		position="static"
		className={classNames(classes.appBar, {
            		[classes.appBarShift]: openDrawer,
          	})}>
          <Toolbar>
            <IconButton
		className={classNames(classes.menuButton, openDrawer && classes.hide)}
		color="inherit"
		aria-label="Open drawer"
		onClick={this.handleDrawerOpen}>
              <MenuIcon />
            </IconButton>
            <Typography variant="h6" color="inherit" noWrap>
                АйВи Консантинг
            </Typography>
            <div className={classes.grow} />
            <div className={classNames(classes.search, classes.hide)}>
              <div className={classes.searchIcon}>
                <SearchIcon />
              </div>
              <InputBase
                placeholder="Search..."
                classes={{
                  root: classes.inputRoot,
                  input: classes.inputInput,
                }}
              />
            </div>
	    <div className={classes.hide}>
                <IconButton
                  aria-owns={open ? 'menu-appbar' : undefined}
                  aria-haspopup="true"
                  onClick={this.handleMenu}
                  color="inherit"
                >
                  <AccountCircle />
                </IconButton>
                <Menu
                  id="menu-appbar"
                  anchorEl={anchorEl}
                  anchorOrigin={{
                    vertical: 'top',
                    horizontal: 'right',
                  }}
                  transformOrigin={{
                    vertical: 'top',
                    horizontal: 'right',
                  }}
                  open={open}
                  onClose={this.handleClose}
                >
                  <MenuItem onClick={this.handleClose}>Profile</MenuItem>
                  <MenuItem onClick={this.handleClose}>My account</MenuItem>
                </Menu>
              </div>
            </Toolbar>
        </AppBar>
        <Drawer
            className={classes.drawer}
            variant="persistent"
            anchor="left"
            open={openDrawer}
            classes={{
                paper: classes.drawerPaper,
            }}
        >
            <div className={classes.drawerHeader}>
                <IconButton onClick={this.handleDrawerClose}>
                    {theme.direction === 'ltr' ? <ChevronLeftIcon /> : <ChevronRightIcon />}
                </IconButton>
            </div>
            <Divider />
            <List>
		<ListItem button component="a" href="/">
			<ListItemIcon><HomeIcon /></ListItemIcon>
    			<ListItemText primary="Домашняя" />
  		</ListItem>
		<ListItem button component="a" href="/products">
			<ListItemIcon><AppsIcon /></ListItemIcon>
    			<ListItemText primary="Проекты" />
  		</ListItem>
		<ListItem button component="a" href="/contacts">
			<ListItemIcon><ContactsIcon /></ListItemIcon>
    			<ListItemText primary="Контакты" />
  		</ListItem>
            </List>
         </Drawer>
	<main
          className={classNames(classes.content, {
            [classes.contentShift]: openDrawer,
          })}
        >
		{this.body}
        </main>
        </div>
    );
  }
}

NavBar.propTypes = {
  classes: PropTypes.object.isRequired,
  theme: PropTypes.object.isRequired,
};

export default withStyles(styles, { withTheme: true })(NavBar);
