<?php

require_once('common.php');

$id = $_GET['id'] ? $_GET['id'] : '0';

$movie_sql = 'SELECT title, year, rating, company
	FROM Movie
	WHERE id = :id
	LIMIT 1
';

$dbh = get_db_handle();
$sth = $dbh->prepare( $movie_sql );
$sth->execute( array( ':id' => $id ) );
$movie = $sth->fetch( PDO::FETCH_ASSOC );

$cast_sql = 'SELECT Actor.id, CONCAT(Actor.first, " ", Actor.last) as Name, MovieActor.role as Role
	FROM Actor
	JOIN MovieActor ON MovieActor.aid = Actor.id
	WHERE MovieActor.mid = :id
';

$director_sql ='SELECT Director.id, CONCAT(Director.first, " ", Director.last) as Name
	FROM Director
	JOIN MovieDirector ON MovieDirector.did = Director.id
	WHERE MovieDirector.mid = :id
';

$sth = $dbh->prepare( $cast_sql );
$sth->execute( array( ':id' => $id ) );
$cast = $sth->fetchAll( PDO::FETCH_ASSOC );

$sth = $dbh->prepare( $director_sql );
$sth->execute( array( ':id' => $id ) );
$directors = $sth->fetchAll( PDO::FETCH_ASSOC );

?>
<!DOCTYPE html>
<html>
	<head>
		<title>CS143 Project 1C - Movie Database</title>
	</head>
	<body>
		<p>
			<strong><?php echo $movie['title'] ?></strong>, <em><?php echo $movie['year']; ?></em>
			<br>
			Rating: <?php echo $movie['rating']; ?>
			<br>
			Produced by: <?php echo $movie['company']; ?>
		</p>

		<?php echo render_table( $directors, PERSON_VIEW, 'id', 'Name', 'Directed By', false ); ?>
		<br>
		<?php echo render_table( $cast, PERSON_VIEW, 'id', 'Name', 'Film Cast' ); ?>
	</body>
</html>
