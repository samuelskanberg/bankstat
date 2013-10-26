<?php

function bankstat_import_form($form, &$form_state) {
	 
	$form['submit_button'] = array(
			'#type' => 'submit',
			'#value' => t('Import'),
			);

	return $form;
}


function bankstat_import_form_submit($form, &$form_state) {

}

