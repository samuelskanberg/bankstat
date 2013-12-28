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

		drupal_set_message(t('The file @filename has been imported', array('@filename' => $file->filename)));
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
	// This will be false since we have not yet set any taxonomies on it. 0 equals false
	$node->field_is_categorized[$node->language][0]['value'] = 0;
	// Chosen bank account
	$node->field_bank_account[$node->language][0]['tid'] = $node_info['bank_account_tid'];

	$node = node_submit($node);
	node_save($node);
}


function bankstat_view_all_form($form, &$form_state) {
	// TODO: Get GET parameter to check if some special stuff has been selected
	$nodes_list = node_load_multiple(array(), array('type' => 'expensetype'));
	
	$nodes = array();

	$count = 0;
	$limit = 1000;

	foreach ($nodes_list as $node) {
		// This will be false since we have not yet set any taxonomies on it. 0 equals false
		$is_categorized = $node->field_is_categorized[$node->language][0]['value'];

		// If it's already categorized, skip it
		if ($is_categorized == 1) {
			continue;
		}

		// So we don't get timeout if there are too many
		$count = $count+1;
		if ($count > $limit) {
			break;
		}

		$nid = $node->nid;

		$title = $node->title;
		$amount = $node->field_amount[$node->language][0]['value'];

		$balance = $node->field_balance[$node->language][0]['value'];
		$booking_date = $node->field_booking_date[$node->language][0]['value'];
		$transaction_date = $node->field_transaction_date[$node->language][0]['value'];
		// Chosen bank account
		$bank_account_tid = $node->field_bank_account[$node->language][0]['tid'];

		///$display = $booking_date . ', ' . $amount . ' - ' . '<label>' . $title . '</label>';
		$display = $booking_date . ', ' . $amount . ' - ' . '<b>' . $title . '</b>';

		// Add to list
		$nodes[$nid] = $display;
	}

	$form['#attached']['js'] = array(
		drupal_get_path('module', 'bankstat') . '/bankstat.js',
	);

	$form['scripttextfield'] = array(
		'#id' => 'scripttextfield',
		'#type' => 'textfield',
		'#title' => t('Script your stuff'),
	);

	$form['applyscriptbutton'] = array(
		'#id' => 'applyscriptbutton',
		'#type' => 'button',
		'#value' => t('Apply rule'),
		//'#attributes ' => array('onclick' => 'applyPatternOnSelect()'),
	);

	$form['selected_nids'] = array(
		'#id' => 'selected_nids',
		'#type' => 'checkboxes',
		'#title' => t('Nodes'),
		'#options' => $nodes,
	);

	// Get a list of all the expense_tags
	$expense_tags_vocabulary = taxonomy_vocabulary_machine_name_load('expense');
	$terms = taxonomy_get_tree($expense_tags_vocabulary->vid);

	$expense_tags_terms = array();

	$expense_tags_terms[0] = '<' . t('Create new term') . '>';

	foreach($terms as $term) {
		$expense_tags_terms[$term->tid] = $term->name;
	}

	$tids = array_keys($expense_tags_terms);
	$default_value = $tids[0];

  $form['expense_tags'] = array(
			'#id' => 'expense_tags',
			'#type' => 'select',
			'#title' => t('Expense tags'),
			'#default_value' => $default_value,
			'#options' => $expense_tags_terms,
	);

	$form['new_tag'] = array(
		'#id' => 'new_tag',
		'#type' => 'textfield',
		'#title' => t('New tag'),
	);

	$form['submit_button'] = array(
		'#type' => 'submit',
		'#value' => t('Set new tags'),
	);

	return $form;
}



function bankstat_view_all_form_submit($form, &$form_state) {
	// Get all selected values

	$selected_nids = array_keys(array_filter($form_state['values']['selected_nids']));

	$expense_tag_dropdown_tid = intval($form_state['values']['expense_tags']);

	if ($expense_tag_dropdown_tid == 0) {
		//echo "new tag used <br/>";
		// Create new term
		$expense_tags_vocabulary = taxonomy_vocabulary_machine_name_load('expense');
		//https://api.drupal.org/api/drupal/modules!taxonomy!taxonomy.module/function/taxonomy_term_save/7
		$term = new stdClass();
		$term->name = $form_state['values']['new_tag'];
		$term->vid = $expense_tags_vocabulary->vid;
		taxonomy_term_save($term);
		$expense_tag_tid = $term->tid;
		//echo "<br/>new tag created with name " . $term->name . ", tid: " . $expense_tag_tid . "<br/>";

	} else {
		$expense_tag_tid = $expense_tag_dropdown_tid;
	}

	//echo "<br/><br/>expense tag tid: " . $expense_tag_tid;
	$term = taxonomy_term_load($expense_tag_tid);


	$tag_name = $term->name;
	$number = count($selected_nids);	

	foreach ($selected_nids as $nid) {
		$node = node_load($nid);
		$node->field_tag[$node->language][0]['tid'] = $expense_tag_tid;
		// Now we can say that this node is categorized
		$node->field_is_categorized[$node->language][0]['value'] = 1;
		node_save($node);
	}

	drupal_set_message(t('Successfully set @number items to @tag', array('@number' => $number, '@tag' => $tag_name)));
}

function bankstat_view_node_expensetype_form($form, &$form_state) {
	// TODO: Load all expense types

	// Get a list of all the expense_tags
	$expense_tags_vocabulary = taxonomy_vocabulary_machine_name_load('expense');
	$terms = taxonomy_get_tree($expense_tags_vocabulary->vid);

	$expense_tags_terms = array();

	// TODO: Add an empty row first

	foreach($terms as $term) {
		$expense_tags_terms[$term->tid] = $term->name;
	}

	$tids = array_keys($expense_tags_terms);
	$default_value = $tids[0];

  $form['expense_tags'] = array(
			'#type' => 'select',
			'#title' => t('Expense tags'),
			'#default_value' => $default_value,
			'#options' => $expense_tags_terms,
			);

	$form['submit_button'] = array(
		'#type' => 'submit',
		'#value' => t('Set new tags'),
	);

	return $form;

	// TODO: Check GET value, if not set use not set


	// TODO: Get GET parameter to check if some special stuff has been selected
	$nodes_list = node_load_multiple(array(), array('type' => 'expensetype'));
	
	$nodes = array();

	$count = 0;

	foreach ($nodes_list as $node) {
		// This will be false since we have not yet set any taxonomies on it. 0 equals false
		$is_categorized = $node->field_is_categorized[$node->language][0]['value'];

		// If it's already categorized, skip it
		if ($is_categorized == 1) {
			continue;
		}

		// So we don't get timeout if there are too many
		$count = $count+1;
		if ($count > 50) {
			break;
		}

		$nid = $node->nid;

		$title = $node->title;
		$amount = $node->field_amount[$node->language][0]['value'];

		$balance = $node->field_balance[$node->language][0]['value'];
		$booking_date = $node->field_booking_date[$node->language][0]['value'];
		$transaction_date = $node->field_transaction_date[$node->language][0]['value'];
		// Chosen bank account
		$bank_account_tid = $node->field_bank_account[$node->language][0]['tid'];

		$display = $booking_date . ', ' . $amount . ' - ' . $title;

		// Add to list
		$nodes[$nid] = $display;
	}

	// TODO: Use tableselect perhaps?

	$form['selected_nids'] = array(
		'#type' => 'checkboxes',
		'#title' => t('Nodes'),
		'#options' => $nodes,
	);

	// Get a list of all the expense_tags
	$expense_tags_vocabulary = taxonomy_vocabulary_machine_name_load('expense');
	$terms = taxonomy_get_tree($expense_tags_vocabulary->vid);

	$expense_tags_terms = array();

	// TODO: Add an empty row first

	foreach($terms as $term) {
		$expense_tags_terms[$term->tid] = $term->name;
	}

	$tids = array_keys($expense_tags_terms);
	$default_value = $tids[0];

  $form['expense_tags'] = array(
			'#type' => 'select',
			'#title' => t('Expense tags'),
			'#default_value' => $default_value,
			'#options' => $expense_tags_terms,
			);

	$form['submit_button'] = array(
		'#type' => 'submit',
		'#value' => t('Set new tags'),
	);

	return $form;
}


// Might be better to create a form, so one can choose what fields
// should be included, and when clicking submit one is passed back
// to the same page so that we know what to render

function bankstat_statistics($form, &$form_state) {

	// TODO: Get GET variables
	
	// TODO: Get chosen tags

	// TODO: Get chosen date intervals

	$all_terms = bankstat_get_all_terms();

	//$expense_tag_tids = array('2', '7', '8');
	$expense_tag_tids = array();

	if (isset($_GET['expense_tags'])) {
		$expense_tag_tids = json_decode($_GET['expense_tags']);
	} else {
		$expense_tag_tids = array_keys($all_terms);
	}


	// Uses the https://drupal.org/project/google_chart_tools module
	$form['my_markup'] = array(
	  '#markup' => '<div id="piechart" style="width: 900px; height: 500px;"></div>',
	);

	$result = bankstat_get_tid_sum($expense_tag_tids);

	//echo "<br/><br/> node result: <br/>";
	//var_dump($result);
	//foreach ($result as $row) {
	//	var_dump($row);
	//	echo "<br/><br/>";
	//}

	//var_dump($result['columns']);

	//die();


	//var_dump($all_terms);
	//die();

	$form['selected_tids'] = array(
		'#type' => 'checkboxes',
		'#title' => t('Terms'),
		'#options' => $all_terms,
		'#default_value' => $expense_tag_tids,
	);

	$form['submit_button'] = array(
		'#type' => 'submit',
		'#value' => t('Show'),
	);

	$settings['chart']['chartOne'] = array(
			'header' => $result['columns'],
			'rows' => array($result['rows']),
			'columns' => array('Utgifter'),
			'chartType' => 'PieChart',
			'containerId' =>  'piechart',
			'options' => array(
				'forceIFrame' => FALSE,
				'title' => 'Utgifter',
				'width' => 800,
				'height' => 400
				)
			);
	//Draw it.
	$ret = draw_chart($settings);



	return $form;
}

function bankstat_statistics_submit($form, &$form_state) {
	$selected_tids = array_keys(array_filter($form_state['values']['selected_tids']));

	drupal_goto(current_path(), array(
		'query' => array(
			'expense_tags' => drupal_json_encode($selected_tids),
		)
	));

}

function bankstat_get_tid_sum($tids) {
	//$query = db_select('node', 'n');
	//$query->join('field_data_field_amount', 'Amount', 'Amount.entity_id = n.nid');
	//$query->join('field_data_field_tag', 'Tag', 'Tag.entity_id = n.nid');
	//$query->join('taxonomy_term_data', 'Term', 'Term.tid = Tag.field_tag_tid');
	//$query->fields('n', array('title'));
	//$query->fields('Amount', array('field_amount_value'));
	//$query->fields('Tag', array('field_tag_tid'));
	//$query->fields('Term', array('name'));
	//$query->condition('n.type', 'expensetype', '=');

	$query = db_select('node', 'n');
	$query->join('field_data_field_amount', 'Amount', 'Amount.entity_id = n.nid');
	$query->join('field_data_field_tag', 'Tag', 'Tag.entity_id = n.nid');
	$query->join('taxonomy_term_data', 'Term', 'Term.tid = Tag.field_tag_tid');
	$query->groupBy('name');
	$query->fields('Term', array('name'));
	$query->fields('Term', array('tid'));
	$query->addExpression('sum(field_amount_value)', 'Total');
	$query->condition('n.type', 'expensetype', '=');
	$query->condition('field_amount_value', '0', '<');

	// Specify specific terms to show
	if (count($tids) > 0) {
		$query->condition('Term.tid', $tids, 'in');
	}

	$query->orderBy('sum(field_amount_value)');

	$query_result = $query->execute()->fetchAll();

	$result = array(
		'columns' => array(),
		'rows' => array(),
		'tids' => array(),
	);

	foreach ($query_result as $row) {
		$result['columns'][] = $row->name;
		// Graphs can't handle negative amounts
		$result['rows'][] = abs(intval($row->Total));
		$result['tids'][] = $row->tid;
	}

	return $result;
}


function bankstat_get_all_terms() {
	$query = db_select('node', 'n');
	$query->join('field_data_field_tag', 'Tag', 'Tag.entity_id = n.nid');
	$query->join('taxonomy_term_data', 'Term', 'Term.tid = Tag.field_tag_tid');
	$query->groupBy('name');
	$query->fields('Term', array('name'));
	$query->fields('Term', array('tid'));
	$query->condition('n.type', 'expensetype', '=');
	$query->orderBy('name');

	$query_result = $query->execute()->fetchAll();

	$result = array();

	foreach ($query_result as $row) {
		$result[$row->tid] = $row->name;
	}

	return $result;
}
