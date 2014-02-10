<?php

require_once('common.php');

/**
 * Builds an HTML <select> with the proper <option> tags
 * @param root_name - name to give the select elements
 * @param $selected - array of values to mark as selected: keys accessed: "day", "month", "year"
 */
function build_date_form( $root_name, $selected = array() ) {
	$html = "<select name=\"$root_name" . '[day]"><option></option>';

	for ( $i = 1; $i <= 31; $i++ ) {
		$html .= "<option value='$i'";

		if ( $i == (int)$selected['day'] )
			$html .= ' selected';

		$html .= ">$i</option>";
	}

	$html .= "</select><select name=\"$root_name" . '[month]"><option></option>';

	for ( $i = 1; $i <= 12; $i++ ) {
		$month = date( 'M', mktime( 0, 0, 0, $i ) );
		$html .= "<option value='$i'";

		if ( $i == (int)$selected['month'] )
			$html .= ' selected';

		$html .= ">$month</option>";
	}

	$html .= "</select><select name=\"$root_name" . '[year]"><option></option>';
	for ( $i = (int)date( 'Y' ); $i >= 1800; $i-- ) {
		$html .= "<option value='$i'";

		if ( $i == (int)$selected['year'] )
			$html .= ' selected';

		$html .= ">$i</option>";
	}

	$html .= '</select>';

	return $html;
}

/**
 * Attempts to insert a new row into the Actor or Director table
 * @param $data - array of data to be inserted
 * @return FALSE on error, otherwise the id of either the newly inserted row or the id of an already existing row
 */
function save_person_in_db( $data ) {

	// SQL query defines
	$actor_find_sql = 'SELECT id
		FROM Actor
		WHERE first = :first and last = :last
		LIMIT 1
	';

	$director_find_sql = 'SELECT id
		FROM Director
		WHERE first = :first and last = :last
		LIMIT 1
	';

	$actor_insert_sql = 'INSERT INTO Actor(id, first, last, dob, dod, sex) VALUES (:id, :first, :last, :dob, :dod, :sex)';
	$director_insert_sql = 'INSERT INTO Director(id, first, last, dob, dod) VALUES (:id, :first, :last, :dob, :dod)';

	// Grabs the next row id when operating normally
	$next_id_sql = 'SELECT MAX(id)+1 FROM MaxPersonID';

	// Grabs the next id if the MaxPersonID table is not initialized
	$next_id_sql_failsafe = 'SELECT MAX(id)
		FROM (
			SELECT MAX(id)+1 as id
			FROM Actor

			UNION

			SELECT MAX(id)+1 as id
			FROM Director
		) as tmp
	';

	// Store an id if there isn't one in the table already
	$next_id_failsafe_insert = 'INSERT INTO MaxPersonID(id) VALUES(:id)';

	// Update the max id when everything is running smoothly
	$update_next_id_sql = 'UPDATE MaxPersonID SET id = :id';

	// Validate we are working with the proper type
	if ( $data['type'] !== 'actor' && $data['type'] !== 'director' )
		return false;

	$find_sql          = ( $data['type'] === 'actor'    ? $actor_find_sql   : $director_find_sql   );
	$alt_find_sql      = ( $data['type'] === 'director' ? $actor_find_sql   : $director_find_sql   );
	$person_insert_sql = ( $data['type'] === 'actor'    ? $actor_insert_sql : $director_insert_sql );

	$sql_args     = array(
		':first' => ucfirst( strtolower( (string)$data['first'] ) ),
		':last'  => ucfirst( strtolower( (string)$data['last']  ) ),
	);

	// Init the db and check for existing rows
	$dbh = get_db_handle();
	$sth = $dbh->prepare( $find_sql );
	if ( ! $sth->execute( $sql_args ) ) {
		return false;
	}
	$id = $sth->fetch( PDO::FETCH_COLUMN, 0 );

	// Row exists, bail
	if ( $id ) {
		return $id;
	}

	// Row doesn't exist, check the alt table if person registered there
	// e.g. credited actor has now become a director
	$sth = $dbh->prepare( $alt_find_sql );
	if ( ! $sth->execute( $sql_args ) ) {
		return false;
	}
	$alt_id = $sth->fetch( PDO::FETCH_COLUMN, 0 );
	$max_id_needs_update = true;

	// We can do this the EasyWay™ or the HardWay™
	if ( $alt_id ) {
		$new_id = $alt_id;
		$max_id_needs_update = false;
	} else {
		$sth = $dbh->prepare( $next_id_sql );
		if ( ! $sth->execute() ) {
			return false;
		}
		$new_id = $sth->fetch( PDO::FETCH_COLUMN, 0 );
	}

	// Looks like we're going to have to do it the HardWay™
	if ( !$new_id ) {
		$sth = $dbh->prepare( $next_id_sql_failsafe );
		if ( ! $sth->execute() ) {
			return false;
		}
		$new_id = $sth->fetch( PDO::FETCH_COLUMN, 0 );

		$sth = $dbh->prepare( $next_id_failsafe_insert );
		if ( ! $sth->execute( array( ':id' => $new_id - 1 ) ) ) {
			return false;
		}
	}

	$data_dob = (array)$data['dob'];
	$data_dod = (array)$data['dod'];

	$dob = mktime( 0, 0, 0, $data_dob['month'], $data_dob['day'], $data_dob['year'] );
	$dod = mktime( 0, 0, 0, $data_dod['month'], $data_dod['day'], $data_dod['year'] );

	if ( -1 == $dob || empty( $data_dob['day'] ) || empty( $data_dob['month'] ) || empty( $data_dob['year'] ) ) {
		$dob = NULL;
	}

	if ( -1 == $dod || empty( $data_dod['day'] ) || empty( $data_dod['month'] ) || empty( $data_dod['year'] ) ) {
		$dod = NULL;
	}

	$insert_args = array(
		':id'    => $new_id,
		':first' => $data['first'],
		':last'  => $data['last'],
		':sex'   => $data['sex'],
		':dob'   => $data_dob['year'] . '-' . $data_dob['month'] . '-' . $data_dob['day'],
		':dod'   => $data_dod['year'] . '-' . $data_dod['month'] . '-' . $data_dod['day'],
	);

	if ( $person_insert_sql != $actor_insert_sql ) {
		unset($insert_args[':sex']);
	}

	$dbh->beginTransaction();
	$sth = $dbh->prepare( $person_insert_sql );
	$person_status = $sth->execute( $insert_args );

	$max_id_status = true;

	if ( $max_id_needs_update && isset( $new_id ) ) {
		$sth = $dbh->prepare( $update_next_id_sql );
		$max_id_status = $sth->execute( array( ':id' => $new_id ) );
	}

	if ( $person_status && $max_id_status ) {
		$dbh->commit();
		return $new_id;
	}

	$dbh->rollback();
	return false;
}

$default_values = array();
$error_messages = array();

if ( isset( $_POST['form'] ) ) {
	$form = $_POST['form'];

	// Check for valid first/last names
	if ( empty( $form['first'] ) && empty( $form['last'] ) ) {
		$error_messages['name'] = '<div class="err-msg">Either a first or last name <em>must</em> be specified.</div>';
	} else {
		$default_values['first'] = (string)$form['first'];
		$default_values['last']  = (string)$form['last'];
	}

	// Check for proper gender input
	if ( !in_array( $form['sex'], array( 'male', 'female' ), true ) ) {
		$error_messages['sex'] = '<div class="err-msg">Invalid gender specified.</div>';
	} else {
		$default_values['sex'] = (string)$form['sex'];
	}

	// Check for proper dates of birth and death
	// Both are optional, but if any input is submitted both dates
	// must be a valid date between now and the cutoff date.
	// dod must also come before dob
	$age_cutoff_date = strtotime( '1/1/1800' );
	$form_dob = (array)$form['dob'];
	$form_dod = (array)$form['dod'];

	$dob_submitted = !empty($form_dob['day']) && !empty($form_dob['month']) && !empty($form_dob['year']);
	$dod_submitted = !empty($form_dod['day']) && !empty($form_dod['month']) && !empty($form_dod['year']);

	$dob = mktime(0, 0, 0, (int)$form_dob['month'], (int)$form_dob['day'], (int)$form_dob['year'] );
	$dod = mktime(0, 0, 0, (int)$form_dod['month'], (int)$form_dod['day'], (int)$form_dod['year'] );

	if ( ($dob_submitted && !$dob) || $dob < $age_cutoff_date || $dob > time() ) {
		$error_messages['dob'] = '<div class="err-msg">Invalid date of birth specified.</div>';
		$dob = false;
	} else {
		$default_values['dob'] = array(
			'day'   => (int)$form_dob['day'],
			'month' => (int)$form_dob['month'],
			'year'  => (int)$form_dob['year'],
		);
	}

	if ( ($dod_submitted && !$dod) || $dob < $age_cutoff_date || $dod > time() ) {
		$error_messages['dod'] = '<div class="err-msg">Invalid date of death specified.</div>';
		$dod = false;
	} else if ( $dob && $dod < $dob ) { // It is okay to submit dod without dob
		$error_messages['dod'] = '<div class="err-msg">Date of death <em>must</em> occur after date of birth</div>';
	} else {
		$default_values['dod'] = array(
			'day'   => (int)$form_dod['day'],
			'month' => (int)$form_dod['month'],
			'year'  => (int)$form_dod['year'],
		);
	}

	// Check for proper type input
	if ( !in_array( $form['type'], array( 'actor', 'director' ), true ) ) {
		$error_messages['type'] = '<div class="err-msg">Invalid type specified.</div>';
	} else {
		$default_values['type'] = (string)$form['type'];
	}

	if ( empty( $error_messages ) ) {
		$new_id = save_person_in_db( $default_values );
		if ( !$new_id ) {
			error_500();
		} else {
			redirect_to( url_for_id( PERSON_VIEW, $new_id ) );
		}
	}
}
page_header( 'Add Person' );
?>
		<p><h3>New person information</h3></p>
		<form action="<?php echo PERSON_ADD; ?>" method="POST">
			<?php if ( isset( $error_messages['name'] ) ) echo $error_messages['name']; ?>
			First name: <input type="text" name="form[first]" value="<?php echo $default_values['first']; ?>">
			<br>
			Last name: <input type="text" name="form[last]" value="<?php echo $default_values['last']; ?>">
			<br>
			<?php if ( isset( $error_messages['sex'] ) ) echo $error_messages['sex']; ?>
			Gender:
				<input type="radio" name="form[sex]" value="male" <?php echo ($default_values['sex'] == 'male' ? 'checked' : '' ); ?> >Male
				<input type="radio" name="form[sex]" value="female" <?php echo ($default_values['sex'] == 'female' ? 'checked' : '' ); ?> >Female
				<br>
			<?php if ( isset( $error_messages['dob'] ) ) echo $error_messages['dob']; ?>
			Date of birth: <?php echo build_date_form( 'form[dob]', $default_values['dob'] ); ?>
			<br>
			<?php if ( isset( $error_messages['dod'] ) ) echo $error_messages['dod']; ?>
			Date of death: <?php echo build_date_form( 'form[dod]', $default_values['dod'] ); ?>
			<br>
			<?php if ( isset( $error_messages['type'] ) ) echo $error_messages['type']; ?>
			Actor or Director?
				<select name="form[type]">
					<option value="actor" <?php echo ($default_values['type'] == 'actor' ? 'selected' : '' ); ?>>Actor</option>
					<option value="director" <?php echo ($default_values['type'] == 'director' ? 'selected' : '' ); ?>>Director</option>
				</select>
			<br>
			<br>
			<input type="submit" value="Submit">
		</form>
<?php page_footer(); ?>