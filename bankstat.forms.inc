<?php

/* ----- PAGES FORMS ----- */

// Import file to database
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
    } else {
        form_set_error('file', t('Error reading file'));
    }
}

// Manually assign node to terms
function bankstat_assign_all_form($form, &$form_state) {
    $limit = 1000;
    $nodes = bankstat_get_untagged_nids_with_description($limit);

    $form['#attached']['js'] = array(
        drupal_get_path('module', 'bankstat') . '/bankstat.js',
    );

    $header = array(
        'date' => 'Date',
        'amount' => 'Amount',
        'title' => 'Title',
    );

    $options = array();

    foreach ($nodes as $node) {
        $options[$node['nid']] = array(
            'date' => $node['date'],
            'amount' => $node['amount'],
            'title' => $node['title'],
        );
    }

    $form['selected_nids'] = array(
        '#type' => 'tableselect',
        '#header' => $header,
        '#options' => $options,
        '#empty' => t('No nodes found'),
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

function bankstat_assign_all_form_submit($form, &$form_state) {
    // Get all selected values
    $selected_nids = array_keys(array_filter($form_state['values']['selected_nids']));

    $expense_tag_dropdown_tid = intval($form_state['values']['expense_tags']);

    if ($expense_tag_dropdown_tid == 0) {
        // Create new term
        $expense_tags_vocabulary = taxonomy_vocabulary_machine_name_load('expense');
        //https://api.drupal.org/api/drupal/modules!taxonomy!taxonomy.module/function/taxonomy_term_save/7
        $term = new stdClass();
        $term->name = $form_state['values']['new_tag'];
        $term->vid = $expense_tags_vocabulary->vid;
        taxonomy_term_save($term);
        $expense_tag_tid = $term->tid;
    } else {
        $expense_tag_tid = $expense_tag_dropdown_tid;
    }

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

// Assign nodes to terms automatically based on what is already in the database
function bankstat_assign_all_automatically_form($form, &$form_state) {
    $limit = 1000;
    $nodes = bankstat_get_untagged_nids_with_description_and_suggested_term($limit);

    $header = array(
        'date' => 'Date',
        'amount' => 'Amount',
        'title' => 'Title',
        'term_name' => 'Suggested term',
        'tid' => 'tid',
    );

    $options = array();

    foreach ($nodes as $node) {
        $options[$node['nid']] = array(
            'date' => $node['date'],
            'amount' => $node['amount'],
            'title' => $node['title'],
            'term_name' => $node['term_name'],
            'tid' => $node['tid'],
        );
    }

    $form['selected_nids'] = array(
        '#type' => 'tableselect',
        '#header' => $header,
        '#options' => $options,
        '#empty' => t('No users found'),
    );

    $form['submit_button'] = array(
        '#type' => 'submit',
        '#value' => t('Set new tags'),
    );

    return $form;
}


function bankstat_assign_all_automatically_form_submit($form, &$form_state) {
    // Get all selected values
    $selected_nids = array_keys(array_filter($form_state['values']['selected_nids']));
    $selected_nids_options = $form['selected_nids']['#options'];

    $number = count($selected_nids);	

    foreach ($selected_nids as $nid) {
            $node = node_load($nid);
            $suggested_tid = $selected_nids_options[$nid]['tid'];

            // Load suggested tid for this nid
            $node->field_tag[$node->language][0]['tid'] = $suggested_tid;
            // Now we can say that this node is categorized
            $node->field_is_categorized[$node->language][0]['value'] = 1;
            node_save($node);
    }

    drupal_set_message(t('Successfully set @number items to suggested tag', array('@number' => $number)));
}

// View statistics about the nodes and their terms in the database
function bankstat_statistics_combined($form, &$form_state) {
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
    $form['linechart'] = array(
        '#markup' => '<div id="linechart" style="width: 800px; height: 600px;"></div>',
    );

    $form['piechart'] = array(
        '#markup' => '<div id="piechart" style="width: 800px; height: 400px;"></div>',
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

    $all_terms_with_links = array();
    // Also make the text for the checkbox a link
    foreach ($all_terms as $tid => $term_name) {
        // TODO: Fix date formatting so we don't need the quatition marks
        $all_terms_with_links[$tid] = '<a href="'. url('bankstat/view/nodes', array('query' => array('term' => $tid, 'start_date' => '"'.$start_date.'"', 'end_date' => '"'.$end_date.'"'))) . '">' . $term_name . '</a>';
    }
    $form['selected_tids'] = array(
        '#type' => 'checkboxes',
        '#title' => t('Terms'),
        '#options' => $all_terms_with_links,
        '#default_value' => $expense_tag_tids,
    );

    $form['submit_button'] = array(
        '#type' => 'submit',
        '#value' => t('Show'),
    );

    // Result for showing the line chart
    $linechartResult = bankstat_get_tid_sum_over_time($expense_tag_tids, $start_date, $end_date);

    $settings['chart']['chartOne'] = array(
        'header' => $linechartResult['header'],
        'rows' => array_values($linechartResult['values']),
        'columns' => array_keys($linechartResult['values']),
        'chartType' => 'LineChart',
        'containerId' =>  'linechart',
        'options' => array(
            'forceIFrame' => FALSE,
            'title' => 'Utgifter',
            'width' => 800,
            'height' => 600
        )
    );


    // Result for showing the sum over the same time
    $selected_months = bankstat_get_all_months_between($start_date, $end_date);
    $piechartResult = bankstat_get_tid_sum($expense_tag_tids, $selected_months);

    $settings['chart']['chartTwo'] = array(
        'header' => $piechartResult['names'],
        'rows' => array($piechartResult['sums']),
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

function bankstat_statistics_combined_submit($form, &$form_state) {
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
        )
    ));
}


// View statistics about the nodes and their terms in the database
function bankstat_view_terms($form, &$form_state) {
    $all_terms = bankstat_get_all_terms();

    $content = '';
    foreach ($all_terms as $tid => $term_name) {
        $content .= '<tr><td>' . '<a href="' . url('bankstat/view/nodes', array('query' => array('term' => $tid))) . '">' . $term_name . '</a>' . '</td></tr>';
    }

    $form['table'] = array(
        '#prefix' => '<table><thead><tr><th>Term</th></tr><tbody>',
        '#markup' => $content,
        '#suffix' => '</tbody></table>',
    );

    return $form;
}

// View nodes
function bankstat_view_nodes($form, &$form_state) {
    $first_date = bankstat_first_date();
    $last_date = bankstat_last_date();

    if (isset($_GET['term'])) {
        $term_tid = json_decode($_GET['term']);
    } else {
        return $form;
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

    $nodes = bankstat_get_nodes(array($term_tid), $start_date, $end_date);

    $content = '';

    foreach ($nodes as $node) {
        $content .= '<tr>';
        $content .= '<td>';
        $content .= $node['nid'];
        $content .= '</td>';
        $content .= '<td>';
        $content .= $node['date'];
        $content .= '</td>';
        $content .= '<td>';
        $content .= $node['title'];
        $content .= '</td>';
        $content .= '<td>';
        $content .= $node['amount'];
        $content .= '</td>';
        $content .= '<td>';
        $content .= $node['term'];
        $content .= '</td>';
        $content .= '</tr>';
    }

    $form['table'] = array(
        '#prefix' => '<table><thead><tr><th>ID</th><th>Date</th><th>Title</th><th>Amount</th><th>Term</th></tr><tbody>',
        '#markup' => $content,
        '#suffix' => '</tbody></table>',
    );

    return $form;
}

/* ----- HELPER FUNCTIONS ----- */
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
            'amount' => str_replace(" ", "", $fields[3]),
            'balance' => str_replace(" ", "", $fields[4]),
            'bank_account_tid' => $bank_account_tid,
        );

        bankstat_create_node($node_info);
    }
    fclose($file);
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


function bankstat_get_nodes($tids, $start_date, $end_date) {
    $query = db_select('node', 'n');
    $query->join('field_data_field_amount', 'Amount', 'Amount.entity_id = n.nid');
    $query->join('field_data_field_tag', 'Tag', 'Tag.entity_id = n.nid');
    $query->join('taxonomy_term_data', 'Term', 'Term.tid = Tag.field_tag_tid');
    $query->join('field_data_field_transaction_date', 'TransactionDate', 'TransactionDate.entity_id = n.nid');
    $query->fields('n', array('nid', 'title'));
    $query->fields('Term', array('name'));
    $query->fields('Term', array('tid'));
    $query->fields('Amount', array('field_amount_value'));
    $query->fields('TransactionDate', array('field_transaction_date_value'));
    $query->condition('n.type', 'expensetype', '=');
    $query->condition('TransactionDate.field_transaction_date_value', $start_date, '>');
    $query->condition('TransactionDate.field_transaction_date_value', $end_date, '<');

    // Specify specific terms to show
    if (count($tids) > 0) {
        $query->condition('Term.tid', $tids, 'in');
    }

    $query_result = $query->execute()->fetchAll();

    $result = array();

    foreach ($query_result as $row) {
        $node = array(
            'nid' => $row->nid,
            'date' => $row->field_transaction_date_value,
            'title' => $row->title,
            'amount' => $row->field_amount_value,
            'tid' => $row->tid,
            'term' => $row->name,
        );
        $result[] = $node;
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

function bankstat_get_untagged_nids_with_description($limit = null) {
    $query = db_select('node', 'n');
    $query->join('field_data_field_transaction_date', 'TransactionDate', 'TransactionDate.entity_id = n.nid');
    $query->join('field_data_field_amount', 'Amount', 'Amount.entity_id = n.nid');
    $query->leftjoin('field_data_field_tag', 'Tag', 'Tag.entity_id = n.nid');
    $query->condition('n.type', 'expensetype', '=');
    $query->isNull('Tag.entity_id');
    $query->fields('n', array('nid', 'title'));
    $query->fields('TransactionDate', array('field_transaction_date_value'));
    $query->fields('Amount', array('field_amount_value'));
    if ($limit != null) {
        $query->range(0, intval($limit)-1);
    }

    $query_result = $query->execute()->fetchAll();

    $result = array();

    foreach ($query_result as $row) {
        $result[] = array(
        'nid' => $row->nid,
        'date' => $row->field_transaction_date_value,
        'amount' => $row->field_amount_value,
        'title' => $row->title,
        );
    }

    return $result;
}

function bankstat_get_untagged_nids_with_description_and_suggested_term($limit = null) {
    $list = bankstat_get_untagged_nids_with_description($limit);

    $list_with_suggestion = array();

    for ($i = 0; $i < count($list); $i++) {
        $title = bankstat_get_title($list[$i]['nid']);
        $suggested_tid = bankstat_guess_tid_to_use($title);

        if ($suggested_tid != 0) {
            $term = taxonomy_term_load($suggested_tid);
            $suggested_term_name = $term->name;
            $array_item = $list[$i];
            // Simply add the values to the array
            $array_item['tid'] = $suggested_tid;
            $array_item['term_name'] = $suggested_term_name;
            $list_with_suggestion[] = $array_item;
        }
    }

    return $list_with_suggestion;
}

function bankstat_get_title($nid) {
    $query = db_select('node', 'n');
    $query->fields('n', array('title'));
    $query->condition('n.nid', $nid, '=');

    $query_result = $query->execute()->fetchAll();

    $title = $query_result[0]->title;

    return $title;
}


function bankstat_guess_tid_to_use($title) {
    $query = db_select('node', 'n');
    $query->join('field_data_field_tag', 'Tag', 'Tag.entity_id = n.nid');
    $query->fields('Tag', array('field_tag_tid'));
    $query->condition('n.type', 'expensetype', '=');
    $query->condition('n.title', $title, '=');

    $query_result = $query->execute()->fetchAll();

    if (count($query_result) > 0) {
        return $query_result[0]->field_tag_tid;	
    } else {
        return 0;
    }
}
