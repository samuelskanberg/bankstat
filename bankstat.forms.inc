<?php

function bankstat_import_form($form, &$form_state) {
	$form['file'] = array(
		'#type' => 'file',
		'#title' => t('Choose a file'),
	);

	$form['submit_button'] = array(
		'#type' => 'submit',
		'#value' => t('Import'),
	);

	return $form;
}

function bankstat_import_form_submit($form, &$form_state) {
	$file = file_save_upload('file', array(
		'file_validate_extensions' => array('csv'),
	));

	if ($file) {
		$realpath = drupal_realpath($file->uri);
		bankstat_import_file_to_database($realpath);

		drupal_set_message(t('The file has been uploaded'));
	}
	else {
		form_set_error('file', t('Error reading file'));
	}
}


function bankstat_import_file_to_database($filepath) {
		$file = fopen($filepath, 'r');
		
		$header_expected = array(
			"Bokf.dat",
			"Trans.dat",
			"Text",
			"Insättning/Uttag",
			"Behållning"
		);

		// First get the header row
		$line = fgets($file);
		// TODO: Validate header row? Now we skip it ...

		while(!feof($file)) {
			$line = fgets($file);
			$fields = explode(";", $line);
			
			// If we get this, it's probably the end of the file
			if (count($fields) != count($header_expected)) {
				break;
			}

			$node_info = array(
				'booking_date' => $fields[0],
				'transaction_date' => $fields[1],
				'title' => $fields[2],
				'amount' => $fields[3],
				'balance' => $fields[4],
			);

			bankstat_create_node($node_info);

			// TODO: Remove
			break;
		}
		fclose($file);

		die();
}


function bankstat_create_node($node_info) {
	print_r($node_info);

	$node = new stdClass();
	$node->type = 'expensetype';

	node_object_prepare($node);

	echo "NODE<br/>";
	var_dump($node);

	$node->title = $node_info['title'];
	$node->language = LANGUAGE_NONE;

	// Custom fields: Amount, balance, booking date, transaction date
	$node->field_amount[$node->language][0]['value'] = 123.5;

	$node = node_submit($node);
	node_save($node);

}
