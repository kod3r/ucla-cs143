<?php

define(PERSON_VIEW, 'person-view.php');

/**
 * Instantiates a PDO object to the db and returns a handle
 */
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

/**
 * Returns a 404 HTTP status code and redirects to the 404 page
 */
function error_404() {
	header( 'HTTP/1.1 404 Not Found' );
?>
<!DOCTYPE html>
<html>
	<head>
		<title>404 - Oops!</title>
	</head>
	<body>
		<h2>404!</h2>
		<p>That page does not exist!</p>
	</body>
</html>
<?php
	die;
}

/**
 * Builds an HTML table for an array of db objects
 * @param $objects - an array of rows to display
 * @param $link_url - a url where the object should be hyperlinked to, (function appends "?id=" and the object's id)
 * @param $id_col - name of the primary key column name for this object
 * @param $link_col - name of the column to hyperlink
 * @param $table_header - optional header for the top of the table
 * @param $skip_col_names - an array of colums to AVOID rendering in HTML
 * @return HTML string
 */
function render_table($objects, $link_url, $id_col, $link_col, $table_header = '', $skip_col_names = array()) {

	if ( empty( $objects ) ) {
		return '';
	}

	$html = '<table border="1">';

	$display_col_names = array_diff_key( $objects[0], $skip_col_names );
	unset( $display_col_names[$id_col] );
	$display_col_names = array_keys( $display_col_names );

	if ( !empty( $table_header) ) {
		$html .= '<tr><th colspan="' . sizeof( $display_col_names ) . '">' . $table_header . '</th></tr>';
	}

	$html .= '<tr>';
	foreach ( $display_col_names as $col ) {
		$html .= "<td><strong>$col</strong></td>";
	}
	$html .= '</tr>';

	foreach ( $objects as $o ) {
		$html .= '<tr>';
		$id = $o[$id_col];

		foreach ( $display_col_names as $col ) {
			$html .= '<td>';

			if ( $id && $link_url && $col === $link_col ) {
				$html .= "<a href='$link_url?id=$id'>" . $o[$col] . '</a>';
			} else {
				$html .= $o[$col];
			}

			$html .= '</td>';
		}

		$html .= '</tr>';
	}

	$html .= '</table>';
	return $html;
}