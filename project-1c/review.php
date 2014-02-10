<?php

require_once( 'common.php' );

if ( !isset( $_GET['id'] ) && !isset( $_POST['review'] ) ) {
	error_404();
}

$dbh = get_db_handle();

if ( isset( $_GET['id'] ) ) {
	$movie_sql = 'SELECT id, title
		FROM Movie
		WHERE id = :id
		LIMIT 1
	';

	$sth = $dbh->prepare( $movie_sql );
	$sth->execute( array( ':id' => $_GET['id'] ) );
	$movie = $sth->fetch( PDO::FETCH_ASSOC );

	if ( empty( $movie ) ) {
		error_404();
	}
} else if ( isset ( $_POST['review'] ) ) {
	$review = $_POST['review'];

	$sth = $dbh->prepare( 'SELECT COUNT(*) FROM Review WHERE name = :name and mid = :mid' );
	$sth->execute( array( ':name' => $review['name'], ':mid' => $review['id'] ) );

	// Bail if this user has already submitted a review
	if ( $sth->fetch( PDO::FETCH_COLUMN, 0) > 0 ) {
		?>
			<!DOCTYPE html>
			<html>
				<head><title>CS143 Project 1C - Movie Database</title></head>
				<body>
					<p>You have already submitted a review!</p>
					<?php echo hyperlink( MOVIE_VIEW, (int)$review['id'], 'Go back' ); ?>
				</body>
			</html>
		<?php
		die;
	}

	// Otherwise process the data
	$sth = $dbh->prepare( 'INSERT INTO Review(name, time, mid, rating, comment) VALUES (:name, NOW(), :id, :rating, :comment)' );
	$sth->execute( array(
		':name'    => (string)$review['name'],
		':id'      => (int)$review['id'],
		':rating'  => min( 5, max( 0, (int)$review['rating'] ) ),
		':comment' => (string)$review['comment'],
	) );

	redirect_to( url_for_id( MOVIE_VIEW, (int)$review['id'] ) );
}
page_header( 'New Review' );
?>
		<form action="<?php echo REVIEW_FORM; ?>" method="POST">
			<p>You are reviewing <strong><?php echo hyperlink( MOVIE_VIEW, $movie['id'], $movie['title'], '_blank' ); ?></strong></p>
			Your name: <input type="text" name="review[name]">
			<br>
			Rating:
			<select name="review[rating]">
				<option></option>
				<option value='5'>5</option>
				<option value='4'>4</option>
				<option value='3'>3</option>
				<option value='2'>2</option>
				<option value='1'>1</option>
				<option value='0'>0</option>
			</select>
			<br>
			Comments:
			<br>
			<textarea name="review[comment]" rows="6" cols="80"></textarea>
			<br>
			<input type="submit" value="Submit">
			<input type="hidden" value="<?php echo $_GET['id']; ?>" name="review[id]">
		</form>
<?php page_footer(); ?>