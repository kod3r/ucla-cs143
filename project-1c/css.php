<?php
/**
 * This file exists as a .php file (instead of a static file) to get around some caching oddities
 * with static files and shared folders on VirtualBox for Windows.
 */
header( 'Content-Type: text/css' );
?>
body {
	font-family: 'Lustria', "HelveticaNeue-Light", "Helvetica Neue Light", "Helvetica Neue", Helvetica, Arial, "Lucida Grande", sans-serif; 
	font-weight: 300;
	font-size: 14px;
	line-height: 18px;
	width: 1100px;
	margin: 0 auto;
	color: #1A2B2B;
}

h1, h2, h3, h4, h5, h6 {
	font-family: 'Lato';
}

header {
	margin-bottom: 40px;
}

header h1 {
	font-size: 24px;
	font-weight: bold;
	width: 400px;
	float: left;
}

nav ul {
	width: 600px;
	float: right;
	font-size: 14px;
}

nav li {
	display: inline;
}

nav li a, nav li a:visited {
	color: #556270;
	font-size: 16px;
}

p, div {
	clear: both;
}

<?php
/**
 * End of file...
 */