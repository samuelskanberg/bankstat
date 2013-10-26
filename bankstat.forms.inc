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
		while(!feof($file)) {
			echo fgets($file). "<br>";
		}
		fclose($file);
}
