<?php
require_once( 'common.php ');

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

		$movie_insert_sql = 'INSERT INTO Movie (id, title, year, rating, company) VALUES(:id, :title, :year, :rating, :company)';
		$stmt = $dbh->prepare( $movie_insert_sql );
		$stmt->execute( array(
			':id'		=> $mmid + 1,
			':title'	=> $_POST['title'],
			':year'		=> $_POST['year'],
			':rating'	=> $_POST['rating'],
			':company'	=> $_POST['company'],
		) );

		$update_id_sql = 'UPDATE MaxMovieID SET id=' . $mmid + 1;
		$stmt = $dbh->prepare( $update_id_sql );
		$stmt->execute();

		$saved = $mmid + 1;
	}
}

page_header( 'Add a Movie' );
?>
<h3>Add a Movie</h3>
<?php if ($error)
	echo '<strong>' . $error . '</strong>';
if ( false !== $saved )
	echo '<p>Movie saved! ' . hyperlink( 'movie-view.php', $saved, 'Click here to view.' ) . '</p>';
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
<input type="submit" name="submit" value="Save">
</form>
<?php
page_footer();