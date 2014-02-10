<?php
require_once( 'common.php' );

page_header( 'Add Relation' );
$dbh = get_db_handle();

if ( ! isset($_POST['submit'] ) ) {
	$movie_sql = 'SELECT title, id FROM Movie ORDER BY title DESC';
	$actor_sql = 'SELECT Actor.id, CONCAT(Actor.first, " ", Actor.last) as Name FROM Actor ORDER BY Actor.last, Actor.first DESC';
	$director_sql = 'SELECT Director.id, CONCAT(Director.first, " ", Director.last) as Name FROM Director ORDER BY Director.last, Director.first DESC';

	$stmt = $dbh->prepare( $movie_sql );
	$stmt->execute();
	$movies = $stmt->fetchAll( PDO::FETCH_ASSOC );

	$stmt = $dbh->prepare( $actor_sql );
	$stmt->execute();
	$actors = $stmt->fetchAll( PDO::FETCH_ASSOC );

	$stmt = $dbh->prepare( $director_sql );
	$stmt->execute();
	$directors = $stmt->fetchAll( PDO::FETCH_ASSOC );
?>
	<div>
		<h3>Add Actor to Movie</h3>
		<form method="post">
			<input type="hidden" name="mode" value="actor">
			Actor: <?php echo generate_select_box( 'actor', $actors, 'id', 'Name' ); ?><br>
			Movie: <?php echo generate_select_box( 'movie', $movies, 'id', 'title' ); ?><br>
			Role: <input type="text" name="role"><br>
			<input type="submit" name="submit" value="Save">
		</form>
	</div>
	<div>
		<h3>Add Director to Movie</h3>
		<form method="post">
			<input type="hidden" name="mode" value="director">
			Director: <?php echo generate_select_box( 'director', $directors, 'id', 'Name' ); ?><br>
			Movie: <?php echo generate_select_box( 'movie', $movies, 'id', 'title' ); ?><br>
			<input type="submit" name="submit" value="Save">
		</form>
	</div>
<?php
} else {
	$mode = $_POST['mode'];

	if ( !in_array( $mode, array('director', 'actor' ) ) )
		die('Invalid mode.');

	if ( $mode == 'director' ) {
		$sql = 'INSERT INTO MovieDirector (mid, did) VALUES(:mid, :did)';
		$stmt = $dbh->prepare( $sql );
		$stmt->execute( array( ':mid' => $_POST['movie'], ':did' => $_POST['director'] ) );
	} else { // Adding an actor, then
		$sql = 'INSERT INTO MovieActor (mid, aid, role) VALUES(:mid, :aid, :role)';
		$stmt = $dbh->prepare( $sql );
		$stmt->execute( array( ':mid' => $_POST['movie'], ':aid' => $_POST['actor'], ':role' => $_POST['role'] ) );
	}

	print 'Relationship saved!';
}

page_footer();
?>