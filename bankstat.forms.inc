<?php

function bankstat_import_form($form, &$form_state) {
	$form['file'] = array(
		'#type' => 'file',
		'#title' => t('Choose a file'),
	);

	// List all the bank accounts
	$bank_account_vocabulary = taxonomy_vocabulary_machine_name_load('bank_account');
	$terms = taxonomy_get_tree($bank_account_vocabulary->vid);

	$bank_account_terms = array();
	foreach($terms as $term) {
		$bank_account_terms[$term->tid] = $term->name;
	}

	$tids = array_keys($bank_account_terms);
	$default_value = $tids[0];

  $form['bank_account'] = array(
			'#type' => 'radios',
			'#title' => t('Bank account'),
			'#default_value' => $default_value,
			'#options' => $bank_account_terms,
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

	$bank_account_tid = $form_state['values']['bank_account'];

	if ($file) {
		$realpath = drupal_realpath($file->uri);
		bankstat_import_file_to_database($realpath, $bank_account_tid);

		drupal_set_message(t('The file has been uploaded'));
	}
	else {
		form_set_error('file', t('Error reading file'));
	}
}


function bankstat_import_file_to_database($filepath, $bank_account_tid) {
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

			// Remove text delimiter
			$line = str_replace("\"", "", $line);

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
				'bank_account_tid' => $bank_account_tid,
			);

			bankstat_create_node($node_info);

			// TODO: Remove
			//break;
		}
		fclose($file);

		//die();
}


function bankstat_create_node($node_info) {
	print_r($node_info);

	$node = new stdClass();
	$node->type = 'expensetype';

	node_object_prepare($node);

	$node->title = $node_info['title'];
	$node->language = LANGUAGE_NONE;

	// Custom fields: Amount, balance, booking date, transaction date
	$node->field_amount[$node->language][0]['value'] = $node_info['amount'];
	$node->field_balance[$node->language][0]['value'] = $node_info['balance'];
	$node->field_booking_date[$node->language][0]['value'] = $node_info['booking_date'];
	$node->field_transaction_date[$node->language][0]['value'] = $node_info['transaction_date'];
	// Chosen bank account
	$node->field_bank_account[$node->language][0]['tid'] = $node_info['bank_account_tid'];

	$node = node_submit($node);
	node_save($node);
}
