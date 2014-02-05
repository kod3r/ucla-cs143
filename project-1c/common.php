<?php

function get_db_handle() {
	global $db_handle;

	if ( ! isset( $db_handle ) || empty( $db_handle ) ) {
		try {
			$db_handle = new PDO('mysql:dbname=CS143', 'cs143');
		} catch (PDOException $e) {
			echo 'Failed to connect to database: ' . $e->getMessage();
			die();
		}
	}

	return $db_handle;
}

function error_404() {
	header( 'HTTP/1.1 404 Not Found' );
	header( 'Location: 404.html');
	die;
}