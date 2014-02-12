<?php
require_once( 'common.php' );

$saved = false;
$error = false;

if ( isset($_POST['submit'] ) ) {
	
	// First, validate...
	if ( empty( $_POST['title'] ) || empty( $_POST['company'] ) ) {
		$error = 'You must specify all fields!';
	} else {
		// Looks like we're good to go!
		$dbh = get_db_handle();
		$mmid_sql = 'SELECT id FROM MaxMovieID';
		$stmt = $dbh->prepare( $mmid_sql );
		$stmt->execute();

		$mmid = $stmt->fetchColumn();

		// If the MaxMovieID table has not been initialized, do so now
		if ( !$mmid ) {
			$stmt = $dbh->prepare( 'SELECT MAX(id) from Movie' );
			$stmt->execute();

			$mmid = $stmt->fetchColumn();

			$stmt = $dbh->prepare( 'INSERT INTO MaxMovieID(id) VALUES(:id)' );
			$stmt->execute( array( ':id' => $mmid + 1 ) );
		}

		$movie_insert_sql = 'INSERT INTO Movie (id, title, year, rating, company) VALUES(:id, :title, :year, :rating, :company)';
		$stmt = $dbh->prepare( $movie_insert_sql );
		$stmt->execute( array(
			':id'		=> $mmid + 1,
			':title'	=> (string)$_POST['title'],
			':year'		=> (int)$_POST['year'],
			':rating'	=> (string)$_POST['rating'],
			':company'	=> (string)$_POST['company'],
		) );

		$update_id_sql = 'UPDATE MaxMovieID SET id=' . $mmid + 1;
		$stmt = $dbh->prepare( $update_id_sql );
		$stmt->execute();

		$saved = $mmid + 1;

		$genres_insert_sql = 'INSERT INTO MovieGenre (mid, genre) VALUES';
		$genres = explode( ',', $_POST['genres'] );
		$genre_args = array();

		for ( $i = 0; $i < sizeof( $genres ); $i++ ) {
			$key = ":g$i";
			$genres_insert_sql .= ",(:mid, $key)";
			$genre_args[$key] = trim( $genres[$i] );
		}

		if ( !empty( $genre_args ) ) {
			$count = 1;
			$genres_insert_sql = str_replace('VALUES,', 'VALUES', $genres_insert_sql, $count);

			$genre_args[':mid'] = $saved;
			$stmt = $dbh->prepare( $genres_insert_sql );
			$stmt->execute( $genre_args );
		}
	}
}

if ( false !== $saved )
	redirect_to( url_for_id( MOVIE_VIEW, $saved ) );

page_header( 'Add a Movie' );
?>
<h3>Add a Movie</h3>
<?php if ($error)
	echo '<strong>' . $error . '</strong>';
?>
<form method="post">
Title: <input type="text" name="title"><br>
Year: <select name="year">
	<?php
	for ( $i = date( 'Y' ); $i >= 1800; $i-- ) {
		echo '<option value="' . $i . ' ">' . $i . '</option>';
	} ?></select><br>
Rating: <select name="rating">
	<?php
	$ratings = array( 'G', 'PG', 'PG-13', 'R', 'NC-17', 'X' );
	foreach ( $ratings as $rating ) {
		echo '<option value="' . $rating . '">' . $rating . '</option>';
		} ?></select><br>
Company: <input type="text" name="company"><br>
Genres (separated by commas): <input type="text" name="genres"><br>
<input type="submit" name="submit" value="Save">
</form>
<?php
page_footer();
