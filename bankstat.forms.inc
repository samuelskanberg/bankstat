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

function bankstat_import_form_validate($form, &$form_state) {
	$file = file_save_upload('file', array(
		'file_validate_extensions' => array('txt'),
	));
	//$file = file_save_upload('csvfile', array('file_validate_extensions' => array()), $filepath, FILE_EXISTS_RENAME);


	if ($file) {
		$realpath = drupal_realpath($file->uri);
		$fh = fopen($realpath, 'r');
		while(!feof($fh))
		{
			echo fgets($fh). "<br>";
		}
		fclose($fh);

		die();

		//var_dump($file);
		//die();
		if ($file = file_move($file, 'public://')) {
			$form_state['values']['file'] = $file;
		} else {
			form_set_error('file', t('Failed to write the uploaded file the site\'s file folder.'));
		}
	}
	else {
		form_set_error('file', t('No file was uploaded.'));
	}
}

function bankstat_import_form_submit($form, &$form_state) {
	$file=$form_state['values']['file'];
	unset($form_state['values']['file']);
	$file->status = FILE_STATUS_PERMANENT;
	file_save($file);
	drupal_set_message(t('The form has been submitted and the file has been saved, filename: @filename.', array('@filename' => $file->filename)));
}

