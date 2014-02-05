<?php

require_once('common.php');

$id = $_GET['id'] ? $_GET['id'] : '0';

$director_sql = 'SELECT last, first, dob, dod
	FROM Director
	WHERE id = :id
	LIMIT 1
';

$dbh = get_db_handle();
$sth = $dbh->prepare( $director_sql );
$sth->execute( array( ':id' => $id ) );
$director = $sth->fetch( PDO::FETCH_ASSOC );

if ( empty( $director ) ) {
	error_404();
}

$movies_sql = 'SELECT Movie.id as id, title as Title, year as Year, rating as `Rating`, company as `Producing Company`
	FROM Movie
	JOIN MovieDirector ON MovieDirector.mid = Movie.id
	JOIN Director ON MovieDirector.did = Director.id
	WHERE Director.id = :id
	ORDER BY year, title
';

$sth = $dbh->prepare( $movies_sql );
$sth->execute( array( ':id' => $id ) );
$movies = $sth->fetchAll( PDO::FETCH_ASSOC );

?>
<!DOCTYPE html>
<html>
	<head>
		<title>CS143 Project 1C - Movie Database</title>
	</head>
	<body>
		<p>
			<?php echo $director['first'] . ' ' . $director['last']; ?>
			<br>
			<?php echo $director['dob'] ? 'Born: ' . date( 'F j, Y', strtotime( $director['dob'] ) ) . '<br>' : ''; ?>
			<?php echo $director['dod'] ? 'Died: ' . date( 'F j, Y', strtotime( $director['dod'] ) ) . '<br>' : ''; ?>
		</p>

		<?php echo render_table( $movies, MOVIE_VIEW, 'id', 'Title', 'Movies Directed', array() ); ?>
	</body>
</html>
