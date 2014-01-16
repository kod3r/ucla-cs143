<?php
/**
 * Configuration
 *
 * These are the only necessary configuration settings. They're
 * configured as-is to run on the course-provided virtual machine.
 */

define( 'DB_NAME', 'CS143' );
define( 'DB_USER', 'cs143' );
define( 'DB_PASS', '' );
define( 'DB_HOST', 'localhost' );

/**
 * This is the actual "application"--no changes should be required
 * below this line.
 */

// "Common" html
?>
<!DOCTYPE html>
<html>
	<head>
		<title>CS143 Project 1B - Query Tool</title>
	</head>
	<body>
		<p>Please enter your query to run here.</p>
		<form action="#" method="GET">
			<textarea name="q" rows="6" cols="80"></textarea>
			<input type="submit" value="Run">
		</form>
<?php

if ( ! empty( $_GET['q'] ) ) {
	echo "<hr>Executing query: <code>{$_GET['q']}</code><br>";

	// Connect to the database and select our working database. The
	// course requires us to use these older functions instead of
	// mysqli_* or PDO.
	$db_handle = mysql_connect( DB_HOST, DB_USER, DB_PASS );
	mysql_select_db( DB_NAME );

	// This really pains my heart...
	$result = mysql_query( $_GET['q'] );
	$error = mysql_error();
	$answer = @mysql_fetch_assoc( $result );
	if ( $error != '' ) {
		print '<strong>An error occurred!</strong> ' . $error;
	} else if ( ! sizeof( $answer ) ) {
		print 'No results found...';
	} else {
		print '<table border="1"><tr>';
		foreach ( array_keys( $answer ) as $col ) {
			print '<td>' . $col . '</td>';
		}
		print '</tr>';
		do {
			print '<tr>';
			foreach ( $answer as $col ) {
				print '<td>' . $col . '</td>';
			}
			print '</tr>';
		} while ( $answer = mysql_fetch_assoc( $result ) );
		print '</table>';
	}
	mysql_free_result( $result );
	mysql_close( $db_handle );

}

// Finish our "template"
?>
	</body>
</html>