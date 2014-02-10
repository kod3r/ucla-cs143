<?php

define(PERSON_ADD, 'person-add.php');
define(PERSON_VIEW, 'person-view.php');
define(MOVIE_VIEW, 'movie-view.php');
define(REVIEW_FORM, 'review.php');

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
 * Returns a 404 HTTP status code and displays an error page
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
 * Returns a 500 HTTP status code and displays an error page
 */
function error_500() {
	header( 'HTTP/1.1 500 Internal Server Error' );
?>
<!DOCTYPE html>
<html>
	<head>
		<title>500 - Oops!</title>
	</head>
	<body>
		<h2>500 Error!</h2>
		<p>Something went wrong :(</p>
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
 * @param $show_headers - display a row of column names
 * @param $skip_col_names - an array of colums to AVOID rendering in HTML
 * @return HTML string
 */
function render_table($objects, $link_url, $id_col, $link_col, $table_header = '', $show_headers = true, $skip_col_names = array()) {

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

	if ( $show_headers ) {
		$html .= '<tr>';
		foreach ( $display_col_names as $col ) {
			$html .= "<td><strong>$col</strong></td>";
		}
		$html .= '</tr>';
	}

	foreach ( $objects as $o ) {
		$html .= '<tr>';
		$id = $o[$id_col];

		foreach ( $display_col_names as $col ) {
			$html .= '<td>';

			if ( $id && $link_url && $col === $link_col ) {
				$html .= hyperlink( $link_url, $id, $o[$col] );
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

/**
 * Appends "?id=" and $id to $url
 */
function url_for_id( $url, $id ) {
	return "$url?id=$id";
}

/**
 * Returns and HTML anchor tag for the given inputs
 * @param $url - url to link to
 * @param $id - parameter to submit as an 'id' GET parameter
 * @param $target
 * @param $text
 */
function hyperlink( $url, $id, $text, $target = NULL ) {
	$a = '<a href="' . url_for_id( $url, $id ) . '"';

	if ( $target )
		$a .= ' target="_blank"';

	$a .= ">$text</a>";
	return $a;
}

/**
 * Redirects to $url and stops execution
 */
function redirect_to( $url ) {
	header( "Location: $url" );
	die;
}

/**
 * Generate the HTML header for each page.
 * @param $title - the page's title
 */
function page_header( $title ) {
?>
<!DOCTYPE html>
<html>
	<head>
		<title>CS143 Project 1C - Movie Database - <?php echo htmlspecialchars( $title ); ?></title>
		<link href='http://fonts.googleapis.com/css?family=Lustria|Lato' rel='stylesheet' type='text/css'>
		<link rel="stylesheet" type="text/css" href="/css.php">
	</head>
	<body>
		<header>
			<h1>Phivan.com Movie Database</h1>
			<nav>
				<ul>
					<li><a href="/person-add.php">Add Person</a></li>
					<li><a href="/movie-add.php">Add Movie</a></li>
					<li><a href="/relation.php">Add Relation</a></li>
					<li><a href="/search.php">Find Person or Movie</a></li>
				</ul>
			</nav>
		</header>
<?php
}

/**
 * Generate the HTML footer for each page
 */
function page_footer() {
?>
	</body>
</html>
<?php
}

/**
 * Generate an HTML select box.
 * @param $name The HTML form name to use
 * @param $values The values to iterate over for the selector
 * @param $key_col The name of the key column
 * @param $display_col The name of the column that holds the displayed value
 */
function generate_select_box( $name, $values, $key_col, $display_col ) {
	$box = '<select name="' . $name . '">';
	foreach ( $values as $value ) {
		$box .= '<option value="' . $value[$key_col] . '">' . $value[$display_col] . '</option>';
	}
	$box .= '</select>';
	return $box;
}