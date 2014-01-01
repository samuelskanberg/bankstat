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


function bankstat_statistics($form, &$form_state) {

	// TODO: Get GET variables
	
	// TODO: Get chosen tags

	// TODO: Get chosen date intervals

	$all_terms = bankstat_get_all_terms();
	$all_months = bankstat_get_all_months();

	$expense_tag_tids = array();

	if (isset($_GET['expense_tags'])) {
		$expense_tag_tids = json_decode($_GET['expense_tags']);
	} else {
		$expense_tag_tids = array_keys($all_terms);
	}

	$selected_months = array();

	if (isset($_GET['months'])) {
		$selected_months = json_decode($_GET['months']);
	} else {
		// Default: latest month
		$selected_months = array(max($all_months));
	}

	// Uses the https://drupal.org/project/google_chart_tools module
	$form['my_markup'] = array(
	  '#markup' => '<div id="chart" style="width: 900px; height: 500px;"></div>',
	);

	$result = bankstat_get_tid_sum($expense_tag_tids, $selected_months);

	//echo "<br/><br/> node result: <br/>";
	//var_dump($result);
	//foreach ($result as $row) {
	//	var_dump($row);
	//	echo "<br/><br/>";
	//}

	//var_dump($result['names']);

	//die();


	//var_dump($all_terms);
	//die();

	//var_dump($all_months);
	//die();

	// To make it associate
	$all_months_options = array_combine($all_months, $all_months);

	$form['selected_months'] = array(
		'#type' => 'checkboxes',
		'#title' => t('Months'),
		'#options' => $all_months_options,
		'#default_value' => $selected_months,
	);

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
			'header' => $result['names'],
			'rows' => array($result['sums']),
			'columns' => array('Utgifter'),
			'chartType' => 'PieChart',
			'containerId' =>  'chart',
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
	$selected_months = array_keys(array_filter($form_state['values']['selected_months']));

	drupal_goto(current_path(), array(
		'query' => array(
			'expense_tags' => drupal_json_encode($selected_tids),
			'months' => drupal_json_encode($selected_months),
		)
	));

}

function bankstat_statistics_linechart($form, &$form_state) {
	$all_terms = bankstat_get_all_terms();

	$first_date = bankstat_first_date();
	$last_date = bankstat_last_date();

	$expense_tag_tids = array();

	if (isset($_GET['expense_tags'])) {
		$expense_tag_tids = json_decode($_GET['expense_tags']);
	} else {
		$expense_tag_tids = array_keys($all_terms);
	}


	if (isset($_GET['start_date'])) {
		$start_date = json_decode($_GET['start_date']);
	} else {
		$start_date = $first_date;
	}

	if (isset($_GET['end_date'])) {
		$end_date = json_decode($_GET['end_date']);
	} else {
		$end_date = $last_date;
	}


	// Uses the https://drupal.org/project/google_chart_tools module
	$form['my_markup'] = array(
	  '#markup' => '<div id="chart" style="width: 800px; height: 600px;"></div>',
	);

	$start_date_times = explode('-', $start_date);
	$start_date_default_value = array(
		'year' => intval($start_date_times[0]),
		'month' => intval($start_date_times[1]),
		'day' => intval($start_date_times[2]),
	);

	$form['start_date'] = array(
		'#type' => 'date',
		'#title' => 'Start date',
		'#default_value' => $start_date_default_value,
	);

	$end_date_times = explode('-', $end_date);
	$end_date_default_value = array(
		'year' => intval($end_date_times[0]),
		'month' => intval($end_date_times[1]),
		'day' => intval($end_date_times[2]),
	);

	$form['end_date'] = array(
		'#type' => 'date',
		'#title' => 'End date',
		'#default_value' => $end_date_default_value,
	);

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

	$result = bankstat_get_tid_sum_over_time($expense_tag_tids, $start_date, $end_date);

	$settings['chart']['chartOne'] = array(
			'header' => $result['header'],
			'rows' => array_values($result['values']),
			'columns' => array_keys($result['values']),
			'chartType' => 'LineChart',
			//'chartType' => 'ColumnChart',
			'containerId' =>  'chart',
			'options' => array(
				'forceIFrame' => FALSE,
				'title' => 'Utgifter',
				'width' => 800,
				'height' => 600
				)
			);
	//Draw it.
	$ret = draw_chart($settings);


	//$foo = array('2001', '2002');
	//$values = array(
	//	array(5, 10),
	//	array(6, 9),
	//);

	//$settings['chart']['chartOne'] = array(
	//		'header' => $foo,
	//		'rows' => $values,
	//		'columns' => array('Utgifter', 'Spify'),
	//		'chartType' => 'LineChart',
	//		'containerId' =>  'chart',
	//		'options' => array(
	//			'forceIFrame' => FALSE,
	//			'title' => 'Utgifter',
	//			'width' => 800,
	//			'height' => 400
	//			)
	//		);
	////Draw it.
	//$ret = draw_chart($settings);

	return $form;
}

function bankstat_statistics_linechart_submit($form, &$form_state) {
	$selected_tids = array_keys(array_filter($form_state['values']['selected_tids']));
	$start_date_a = $form_state['values']['start_date'];
	$end_date_a = $form_state['values']['end_date'];

	$start_date = new DateTime($start_date_a['year'] . '-' . $start_date_a['month'] . '-' . $start_date_a['day']);
	$start_date = $start_date->format('Y-m-d');

	$end_date = new DateTime($end_date_a['year'] . '-' . $end_date_a['month'] . '-' . $end_date_a['day']);
	$end_date = $end_date->format('Y-m-d');

	drupal_goto(current_path(), array(
		'query' => array(
			'expense_tags' => drupal_json_encode($selected_tids),
			'start_date' => drupal_json_encode($start_date),
			'end_date' => drupal_json_encode($end_date),
			//'end_date' => $end_date,
		)
	));
}

function bankstat_get_tid_sum($tids, $months) {
	$query = db_select('node', 'n');
	$query->join('field_data_field_amount', 'Amount', 'Amount.entity_id = n.nid');
	$query->join('field_data_field_tag', 'Tag', 'Tag.entity_id = n.nid');
	$query->join('taxonomy_term_data', 'Term', 'Term.tid = Tag.field_tag_tid');
	$query->join('field_data_field_transaction_date', 'TransactionDate', 'TransactionDate.entity_id = n.nid');
	$query->groupBy('name');
	$query->fields('Term', array('name'));
	$query->fields('Term', array('tid'));
	$query->addExpression('sum(field_amount_value)', 'Total');
	$query->condition('n.type', 'expensetype', '=');
	$query->havingCondition('Total', 0, '<');


	// Specify specific terms to show
	if (count($tids) > 0) {
		$query->condition('Term.tid', $tids, 'in');
	}

	// Specify start date and end date
	if (count($months) > 0) {
		$min_month = min($months);
		$max_month = max($months);

		$start_date = new DateTime($min_month . '-01');
		$start_date = $start_date->format('Y-m-d');
		$end_date = new DateTime($max_month . '-01');
		$end_date->modify('first day of next month');
		$end_date = $end_date->format('Y-m-d');

		$query->condition('TransactionDate.field_transaction_date_value', $start_date, '>');
		$query->condition('TransactionDate.field_transaction_date_value', $end_date, '<');
	}

	$query->orderBy('sum(field_amount_value)');

	$query_result = $query->execute()->fetchAll();

	$result = array(
		'names' => array(),
		'sums' => array(),
		'tids' => array(),
	);

	foreach ($query_result as $row) {
		$result['names'][] = $row->name;
		// Graphs can't handle negative amounts
		$result['sums'][] = abs(intval($row->Total));
		$result['tids'][] = $row->tid;
	}

	return $result;
}


function bankstat_get_tid_sum_over_time($tids, $start_date, $end_date) {
	//$all_months = bankstat_get_all_months();
	$all_months = bankstat_get_all_months_between($start_date, $end_date);

	// Create first column, the x axis
	$header = array();


	// Create a column for each term name
	// For y axis
	$stat = array();
	foreach ($tids as $tid) {
		$term = taxonomy_term_load($tid);
		$stat[$term->name] = array();;
	}

	$stat_keys = array_keys($stat);

	//foreach (array_slice($all_months, 0, 5) as $month) {
	foreach ($all_months as $month) {
		// Make it a string
		$header[] = '' . $month;
		// Set zero for all columns so that all have a value, even if they are not used
		for ($i = 0; $i < count($stat_keys); $i++) {
			$stat[$stat_keys[$i]][] = 0;
		}

		$stat_for_month = bankstat_get_tid_sum($tids, array($month));

		for ($i = 0; $i < count($stat_for_month['names']); $i++) {
			$name = $stat_for_month['names'][$i]; 

			// Add the sum last in the array
			$stat[$name][count($stat[$name])-1] = $stat_for_month['sums'][$i];
		}
		// TODO: Some how add them together. However, only removing break won't work
		//break;	
	}

	$result = array(
		'header' => $header,
		'values' => $stat,
	);

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


function bankstat_get_all_months() {
	$query = db_select('node', 'n');
	$query->join('field_data_field_tag', 'Tag', 'Tag.entity_id = n.nid');
	$query->join('field_data_field_transaction_date', 'TransactionDate', 'TransactionDate.entity_id = n.nid');
	$query->addExpression('SUBSTRING(TransactionDate.field_transaction_date_value, 1, 7)', 'YearMonth');
	$query->groupBy('SUBSTRING(TransactionDate.field_transaction_date_value, 1, 7)');
	$query->condition('n.type', 'expensetype', '=');

	$query_result = $query->execute()->fetchAll();

	$result = array();

	foreach ($query_result as $row) {
		$result[] = $row->YearMonth;
	}

	return $result;
	
}

function bankstat_get_all_months_between($start_date_str, $end_date_str) {
	$months = array();

	$date_iterator = new DateTime($start_date_str);
	$end_date = new DateTime($end_date_str);

	while ($date_iterator < $end_date) {
		$months[] = $date_iterator->format('Y-m');
		$date_iterator->modify('first day of next month');
	}

	return $months;
}

function bankstat_first_date() {
	$query = db_select('node', 'n');
	$query->join('field_data_field_tag', 'Tag', 'Tag.entity_id = n.nid');
	$query->join('field_data_field_transaction_date', 'TransactionDate', 'TransactionDate.entity_id = n.nid');
	$query->addExpression('min(TransactionDate.field_transaction_date_value)', 'date');
	$query->condition('n.type', 'expensetype', '=');

	$query_result = $query->execute()->fetchAll();

	if (count($query_result) > 0) {
	return $query_result[0]->date;
	} else {
		return '1900-01-01';
	}
}

function bankstat_last_date() {
	$query = db_select('node', 'n');
	$query->join('field_data_field_tag', 'Tag', 'Tag.entity_id = n.nid');
	$query->join('field_data_field_transaction_date', 'TransactionDate', 'TransactionDate.entity_id = n.nid');
	$query->addExpression('max(TransactionDate.field_transaction_date_value)', 'date');
	$query->condition('n.type', 'expensetype', '=');

	$query_result = $query->execute()->fetchAll();

	if (count($query_result) > 0) {
	return $query_result[0]->date;
	} else {
		return '2100-01-01';
	}
}
