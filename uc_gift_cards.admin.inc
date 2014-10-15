<?php

/** 
 * @file
 * Administrative functions and callbacks for UC Gift Cards
 *  Contains functions for the admin pages as well as the user admin features.
 */

/**
 * Admin Page Callback
 * 
 * This builds the page at admin/store/gift_cards by pulling in the associated
 * forms and adding markup.
 */
function _uc_gift_cards_admin() {
  //drupal_add_library('uc_gift_cards', 'uc_gift_cards');
  // Display all gift cards in a table?
  $markup = '<h1>Gift Cards Administration</h1>';
  // Display Active/Inactive Link
    if (isset($_REQUEST['active'])) {
    $active = check_plain($_REQUEST['active']);
    $markup .= '<div class="button-link"><a href="/admin/store/gift_cards">Display Active Cards</a></div>';
    $markup .= '<p>Displaying cards that have been archived and are no longer in use.</p>';
      if ($active != 0) {
        // We will only accept 0 as a valid value for $active
        $active = 0;
      }
  } else { 
    $active = 1;
    $markup .= '<div class="button-link"><a href="/admin/store/gift_cards?active=0">Display Inactive Cards</a></div>';
  }
  // Add Search Form
  $gc_search_form = drupal_get_form('uc_gift_cards_admin_search_form');
  $markup .= drupal_render($gc_search_form);
  // Add Create New Form
  $gc_create_form = drupal_get_form('uc_gift_cards_admin_create_new_form');
  $markup .= drupal_render($gc_create_form);
  // Start our Table Display Generation
  $dbh = pdoConnect();
  // Prepare our select statement
  $stmt = $dbh->prepare('SELECT * FROM gift_cards WHERE active = :active');
  $stmt->execute(array(
    ':active' => $active,
  ));

  // Return a simple array of all GC as objects
  $gift_cards = $stmt->fetchAll(PDO::FETCH_CLASS, 'GiftCard');

  // Create a table for our Cards display
  $gc_table['header'] = array(
    array('data' => 'Code', 'data-sort' => 'string', 'class' => 'gc-sortable'),
    array('data' => 'Balance', 'data-sort' => 'float', 'class' => 'gc-sortable'), 
    array('data' =>'Owner', 'data-sort' => 'string', 'class' => 'gc-sortable'),
    array('data' =>'Activated?', 'data-sort' => 'string', 'class' => 'gc-sortable'),
    array('data' =>'Purchase Amount', 'data-sort' => 'float', 'class' => 'gc-sortable'),
    array('data' =>'Order #', 'data-sort' => 'int', 'class' => 'gc-sortable'),
    array('data' =>'Order Date', 'data-sort' => 'int', 'class' => 'gc-sortable'),
    );
  $gc_table['rows'] = array();
  $gc_table['attributes'] = array('id' => 'gift-cards-admin-table');
  $gc_table['caption'] = 'Gift Cards';   
  $gc_table['colgroups'] = array();
  $gc_table['sticky'] = TRUE;
  $gc_table['empty'] = 'No Gift Cards Were Found!';  
  foreach ($gift_cards as $card) {
    if (UC_GIFT_CARDS_SITE_NAME == $card->getSite() && $card->getOrderNumber() != '0') {
      $order_number = '<a href="/admin/store/orders/' . $card->getOrderNumber() . '">' . $card->getOrderNumber() . '</a>';
    } else {
      $order_number = $card->getOrderNumber() . ' (' . $card->getSite() . ')';
    }
    $date_raw = $card->getPurchaseDate();
    $date_formated = date("m-d-Y", $date_raw);
    $gc_email = uc_gift_cards_return_owner($card);
    $code = $card->getCode();
    $code_link = '<a href="/admin/store/gift_cards/' . $card->getID() . '">' . $code . '</a>';
    $gc_table['rows'][] = array(
      $code_link,
      $card->getBalance(),      
      $gc_email,
      $card->isPinSet() == TRUE ? 'Yes': 'No',
      $card->getPurchaseAmount(),
      $order_number,
      array('data-sort-value' => $date_raw, 'data' => $date_formated, 'no_striping' => TRUE),
    );
  }
  
  $markup .= theme_table($gc_table);  
  
  $content['raw_markup'] = array(
    '#type' => 'markup',
    '#markup' => $markup
  );
  return $content;
}

/**
 * Admin Page  Search Form Builder
 */
function uc_gift_cards_admin_search_form($form, &$form_state) {
  $form['card_search'] = array(
    '#type' => 'fieldset',
    '#title' => t('Gift Card Search'),
    '#description' => t('Search for a specific Gift Card Code.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['card_search']['gift_code'] = array(
    '#type' => 'textfield',
    '#title' => t('Gift Card Code'),
    '#description' => t('Enter the Gift Card Code to Search for.'),
    '#name' => 'gift_code',
    '#maxlength' => UC_GIFT_CARDS_CODE_LENGTH,
    '#size' => UC_GIFT_CARDS_CODE_LENGTH * 2,
   );
  $form['card_search']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#submit' => array('uc_gift_cards_admin_search_form_callback'),
    '#ajax' => array('callback' => 'uc_gift_cards_admin_search_form_callback', 'wrapper' => 'card-search-results')
  );
  $form['card_search']['results'] = array(
    '#type' => 'item',
    '#markup' => '<div id="card-search-results"></div>'
  );
  return $form;
}

/**
 * Admin Page Create New Gift Card Form Builder
 */
function uc_gift_cards_admin_create_new_form($form, &$form_state) {
  $form['create'] = array(
    '#type' => 'fieldset',
    '#title' => t('Create New Gift Card'),
    '#description' => t('Allows admins to create a new gift card.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['create']['code'] = array(
    '#type' => 'textfield',
    '#title' => t('Gift Card Code'),
    '#description' => t('Leave blank for auto generated code. Note that code letters will generated as uppercase.'),
    '#name' => 'code',
    '#maxlength' => UC_GIFT_CARDS_CODE_LENGTH,
    '#size' => UC_GIFT_CARDS_CODE_LENGTH * 2
  );
  $form['create']['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Amount'),
    '#description' => t('Enter the Gift Card Amount.'),
    '#name' => 'amount',
    '#default_value' => '10.00',
    '#size' => 12,
   );
  $form['create']['email'] = array(
    '#type' => 'textfield',
    '#title' => t('Owner Email Address'),
    '#description' => t('You may leave this blank, or assign this new Gift Card to an owner by entering their email address here.<br>Note that the default message set in Gift Card Settings will be sent to this email address.'),
    '#name' => 'email',
    '#size' => 36,
  );
  $form['create']['message'] = array(
    '#type' => 'textarea',
    '#title' => t('Message'),
    '#description' => t('Enter a message here to add to the default message sent to the recipient. You must include the email address above.'),
    '#name' => 'message',
    '#cols' => 60,
  );
  $form['create']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#submit' => array('uc_gift_cards_admin_create_new_callback'),
    '#ajax' => array('wrapper' => 'uc-gift-cards-admin-create-new-form')
  );
  return $form;
}

/**
 * Callback Function for Admin Create New Gift Card Form
 */
function uc_gift_cards_admin_create_new_callback(&$form, &$form_state) {
  $amount = check_plain($form_state['input']['amount']);
  if (empty($amount) || $amount < 0 || !is_numeric($amount)) {
    form_set_error('amount', t('Please enter a dollar amount.'));
    $form_state['rebuild'] = TRUE;
    return $form;
  }
  $gift_card = new GiftCard();
  // Generate a code, or check code as entered
  $code = strtoupper(check_plain($form_state['input']['code']));
  if (empty($code)) {
    // Generate Random Code
    $code = $gift_card->genCode();
    $gift_card->setCode($code);
  } else {
    // Verify Code entered is long enough and does not already exist
    if (strlen($code) != UC_GIFT_CARDS_CODE_LENGTH) {
      form_set_error('code', t('Gift Card Code must be '.UC_GIFT_CARDS_CODE_LENGTH.' characters long.'));
      $form_state['rebuild'] = TRUE;
      return $form;
    }
    // Verify code does not exist already
    if (uc_gift_cards_get_from_db($code)) {
      form_set_error('code', t('This code already exists!'));
      $form_state['rebuild'] = TRUE;
      return $form;
    }
    // Go ahead and set the code then
    $gift_card->setCode($code);
  }
  // Set Purchaser to the logged in user
  global $user;
  $purchaser = array(
    'uid' => $user->uid,
    'email' => $user->mail
  );
  $gift_card->setPurchaser($purchaser);
  // See about the email address and message
  $email = check_plain($form_state['input']['email']);
  if (!empty($email) && !valid_email_address($email)) {
    form_set_error('email', t('Please enter a valid email address!'));
    $form_state['rebuild'] = TRUE;
    return $form;  
  }
  $message = check_plain($form_state['input']['message']);
  if (empty($email) && $message) {
    form_set_error('email', t('If you supply a message, you must also supply an email address!'));
    $form_state['rebuild'] = TRUE;
    return $form;
  }
  // Set the amount, date, site, balance
  $gift_card->setPurchaseAmount($amount);
  $gift_card->setBalance($amount);
  $gift_card->setPurchaseDate(time());
  $gift_card->setSite();
  $gift_card->setOrderNumber(0);
  $gift_card->setActive(1);
  
  if ($email) {
    // Set the owner
    $gift_card->setOwner(array('email' => array($email)));
        //dpm($gift_card);
    // then send them an email
    uc_gift_cards_email_code($gift_card, $message);
  } else {
    // set the owner to 0
    $gift_card->setOwnerSimple('[0]');
  }
  // Save to DB 
  $gift_card->saveNew();
  // Create Transaction record
  $notes = "Gift Card $code created through admin form Create New Gift Card by $user->name";
  uc_gift_cards_create_transaction($gift_card->getID(), 0, $amount, 'Created', $notes);
 drupal_set_message(t('New Gift Card ') . $code . t(' Generated in the amount of $') . $amount);
 if ($email) {
   drupal_set_message(t('Notification email was sent to ') . $email);
 }
 return;
}

/**
 * Callback function for Admin Gift Card Search Form
 *  Returns the result of a search for a single gift card code
 * @todo Enable a partial code search?
 */
function uc_gift_cards_admin_search_form_callback($form, &$form_state) {
  $code = strtoupper(check_plain($form_state['input']['gift_code']));
  if (empty($code) || strlen($code) != UC_GIFT_CARDS_CODE_LENGTH) {
    $form_state['rebuild'] = TRUE;
    $form['card_search']['results'] = array(
      '#type' => 'item',
      '#markup' => '<div id="card-search-results">Please enter a ' . UC_GIFT_CARDS_CODE_LENGTH . ' character code to search for.</div>'
      );
    return $form['card_search']['results'];
  }
  // Get our Gift Card Object
  $gift_card = uc_gift_cards_get_card_by_code($code);
  // Create a table for our Cards display
  $gc_table['header'] = array('Code', 'Balance', 'Owner', 'Activated?', 'Purchase Amount', 'Order #', 'Order Date', );
  $gc_table['rows'] = array();
  $gc_table['attributes'] = array();
  $gc_table['caption'] = 'Gift Cards';   
  $gc_table['colgroups'] = array();
  $gc_table['sticky'] = TRUE;
  $gc_table['empty'] = 'No Gift Cards Found.';  
 if ($gift_card) {
    if (UC_GIFT_CARDS_SITE_NAME == $gift_card->getSite() && $gift_card->getOrderNumber() != '0') {
      $order_number = '<a href="/admin/store/orders/' . $gift_card->getOrderNumber() . '">' . $gift_card->getOrderNumber() . '</a>';
    } else {
      $order_number = $gift_card->getOrderNumber() . ' (' . $gift_card->getSite() . ')';
    }
    $gc_email = $gift_card->getOwnerEmail();
    $code = $gift_card->getCode();
    $code_link = '<a href="/admin/store/gift_cards/' . $gift_card->getID() . '">' . $code . '</a>';
    if ($gc_email == FALSE) {
      $gc_email = 'None recorded';
    }
    $gc_table['rows'][] = array(
      $code_link,
      $gift_card->getBalance(),      
      $gc_email,
      $gift_card->isPinSet() == TRUE ? 'Yes': 'No',
      $gift_card->getPurchaseAmount(),
      $order_number,
      date("m-d-Y", $gift_card->getPurchaseDate()),
    );
    $search_results = theme_table($gc_table);
 } else {
   $search_results = 'No Gift Cards Found with the Code <strong>' . $code . '</strong>';
 }
  $form['card_search']['results'] = array(
      '#type' => 'item',
      '#markup' => '<div id="card-search-results">' . $search_results . '</div>'
  );
  return $form['card_search']['results'];
}

/**
 * Callback function to display Gift Card Details
 * @param $id
 *  Gift Card id ( gift_cards.id )
 */
function uc_gift_card_code_view($id) {
  drupal_add_library('uc_gift_cards', 'uc_gift_cards');
  $markup = '<h1>Gift Cards Details</h1>';
  $markup .= '<div class="gc-link"><a href="/admin/store/gift_cards">Return to Gift Card List</a></div>';
  
  // Get our Gift Card Object
  $gift_card = uc_gift_cards_get_card_by_id($id);

  // Get transactions
  $transactions = uc_gift_cards_get_transactions_by_id($gift_card->getID());

  // Get Owner Info
  $dbh = pdoConnect();
  $owner_array = $gift_card->getOwner(); 
  $stmt = $dbh->prepare('SELECT * FROM users WHERE id = :id');
  foreach ($owner_array as $owner_id) {
    $stmt->execute(array(':id' => $owner_id));
    $owner[] = $stmt->fetch();
  }
  // Get Purchaser Info
  $stmt->execute(array(':id' => $gift_card->getPurchaser()));
  $purchaser = $stmt->fetch();
  if ($gift_card->isPinSet()) { 
    $gc_set = 'Yes'; 
  } else { 
    $gc_set = 'No';    
  }
  $markup .= '<div class="gc-code"><h3>Gift Card Code <span class="gc-code-span">' . $gift_card->getCode() . '</span></h3>';
  $markup .= '<div class="gc-code-details"><span class="gc-details-label">Balance:</span> $' . $gift_card->getBalance() . '</div>';
  $markup .= '<div class="gc-code-details"><span class="gc-details-label">Activated?</span> ' . $gc_set . '</div>';
/*** TRANSACTIONS ***/
  // Create a table for our Transactions display
  $trans_table['header'] = array('Order Number', 'Site', 'Amount', 'Trans Type', 'Date', 'User', 'IP', 'Notes');
  $trans_table['rows'] = array();
  $trans_table['attributes'] = array('id' => 'uc_gift_cards_admin_transactions');
  $trans_table['caption'] = t('Gift Card Transactions');   
  $trans_table['colgroups'] = array();
  $trans_table['sticky'] = TRUE;
  $trans_table['empty'] = 'No Transactions Were Found For This Gift Card.'; 
  
  foreach ($transactions as $row) {
    /** @todo Set User and order number to a link if it's on the same site 
     * if (UC_GIFT_CARDS_SITE_NAME == $row['site']) then make link
     */
    $row['order_number'] == '0' ? $order_number = t('N/A') : $order_number = $row['order_number'];
    $trans_table['rows'][] = array(
      $order_number,
      $row['site'],
      $row['amount'],
      $row['code'],
      date("m-d-y \@ h:i A", $row['timestamp']),
      $row['uid'],
      $row['ip'],
      $row['notes']
      );
  }
  $markup .= theme_table($trans_table);

  /*** OWNER INFO ***/
  $markup .= '<div id="owner-info"><h3>Owner Information</h3>';
  foreach ($owner as $owner_site) {
    $markup .= '<div class="gc-details"><span class="gc-details-label">Site:</span> ' . $owner_site['site'] . '</div>';
    $markup .= '<div class="gc-details"><span class="gc-details-label">Email:</span> ' . $owner_site['email'] . '</div>';
    //$markup .= '<div class="gc-details">Site User ID#: ' . $owner['uid'] . '</div>';
    if ($owner_site['site'] == UC_GIFT_CARDS_SITE_NAME && $owner_site['uid']) {
     $owner_site_user = user_load($owner_site['uid']);
     $markup .= '<div class="gc-details"><span class="gc-details-label">Site User ID:</span> <a href="/users/' . $owner_site_user->name . '">' . $owner_site_user->name . '</a></div><br>';
    } else {
     $markup .= '<div class="gc-details"><span class="gc-details-label">Site User ID:</span> None</div><br>';
    }
  }
  $markup .= '</div>'; 
  
  /*** PURCHASE INFO ***/
  $markup .= '<div id="purchaser-info"><h3>Purchaser Information</h3>';
  $markup .= '<div class="gc-details"><span class="gc-details-label">Date Purchased:</span> ' . date("m-d-y \@ h:i A", $gift_card->getPurchaseDate()) . '</div>';
  $markup .= '<div class="gc-details"><span class="gc-details-label">From Site:</span> ' . $gift_card->getSite() . '</div>';
  $markup .= '<div class="gc-details"><span class="gc-details-label">Purchase Amount:</span> $' . $gift_card->getPurchaseAmount() . '</div>';
  $markup .= '<div class="gc-details"><span class="gc-details-label">Purchase Order#:</span> ' . $gift_card->getOrderNumber() . '</div>';
  if ($gift_card->getSite() == UC_GIFT_CARDS_SITE_NAME) {
    $purchaser_site_user = user_load($purchaser['uid']);
    $markup .= '<div class="gc-details"><span class="gc-details-label">Purchaser User:</span> <a href="/users/' . $purchaser_site_user->name . '">' . $purchaser_site_user->name . '</a></div>';
  }
  $markup .= '<div class="gc-details"><span class="gc-details-label">Purchaser Email:</span> ' . $purchaser['email'] . '</div></div>';
  
  $markup .= '</div>'; // End of <div class="gc-code">
  /*** ADMIN OPERATIONS ***/
  $markup .= '<div class="gc-operations"><h1>Card Operations</h1>';
    $archive_form = drupal_get_form('uc_gift_cards_archive_card_form', $gift_card);
  $markup .= drupal_render($archive_form);
    $balance_form = drupal_get_form('uc_gift_cards_adjust_balance_form', $gift_card);
  $markup .= drupal_render($balance_form);
    $pin_reset_form = drupal_get_form('uc_gift_cards_reset_pin_form', $gift_card);
  $markup .= drupal_render($pin_reset_form);
    $pin_set_form = drupal_get_form('uc_gift_cards_set_pin_form', $gift_card);
  $markup .= drupal_render($pin_set_form);
    $delete_card_form = drupal_get_form('uc_gift_cards_admin_delete_card_form', $gift_card->getID());
  $markup .= drupal_render($delete_card_form);
  
  $content['raw_markup'] = array(
    '#type' => 'markup',
    '#markup' => $markup
  );
  return $content;
}

/**
 * Form Builder Function
 * @param object $gift_card
 */
function uc_gift_cards_adjust_balance_form($form, &$form_state, $gift_card) {
  $form['balance'] = array(
    '#type' => 'fieldset',
    '#title' => t('Adjust this Gift Card\'s Balance'),
    '#description' => t('This form allows you to adjust a Gift Card\'s Balance by adding a Credit or Debit to the card balance.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['balance']['current_balance_display'] = array(
    '#type' => 'item',
    '#title' => t('Current Balance'),
    '#markup' => '$' . $gift_card->getBalance()
  );
  $form['balance']['current_balance'] = array(
    '#type' => 'hidden',
    '#value' => $gift_card->getBalance(),
  );
  $form['balance']['adjust'] = array(
    '#type' => 'radios',
    '#title' => t('Credit or Debit'),
    '#default_value' => 0,
    '#options' => array(0 => 'Credit', 1 => 'Debit'),
    '#description' => t('Select Credit to add the amount to the balance, or Debit to subtract it.')
  );
  $form['balance']['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Amount'),
    '#description' => t('Enter the amount to adjust by.'),
    '#name' => 'amount',
    '#size' => 8,
    '#required' => TRUE
      );
  $form['balance']['notes'] = array(
    '#type' => 'textarea',
    '#title' => t('Notes'),
    '#description' => t('Please explain the reason for this adjustment.'),
    '#name' => 'notes',
    '#required' => TRUE,
    '#cols' => 60,
    '#rows' => 4 
  );
  $form['balance']['gift_card_id'] = array(
    '#type' => 'hidden',
    '#value' => $gift_card->getID(),
  );
  $form['balance']['submit'] =array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#submit' => array('uc_gift_cards_adjust_balance_callback'),
    '#ajax' => array('wrapper' => 'uc-gift-cards-adjust-balance-form')
  );
  return $form;
}

/**
 * Callback for the Adjust Balance Form
 */
function uc_gift_cards_adjust_balance_callback(&$form, &$form_state) {
  $card_id = $form_state['input']['gift_card_id'];
  $current_balance = $form_state['input']['current_balance'];
  $adjust = $form_state['input']['adjust'];
  $amount = check_plain($form_state['input']['amount']);
  $notes = check_plain($form_state['input']['notes']);
  // Validation
  if ($amount < 0 || !is_numeric($amount)) {
    form_set_error('amount', 'UPDATE BALANCE FAILED! Please enter the dollar amount to adjust by, without $ or + or -');
    return $form;
  }
  if ($adjust == '0') {
    $new_balance = $current_balance + $amount;
    $code = 'Credit';
  } else {
    $new_balance = $current_balance - $amount;
    $code = 'Debit';
  }
  // Database Connect
  $dbh = pdoConnect();
  // Update Gift Card Balance
  $stmt = $dbh->prepare('UPDATE gift_cards SET balance = :balance WHERE id = :id');
  $stmt->execute(array(
    ':balance' => $new_balance, 
    ':id' => $card_id
      ));
  // Create transaction record
  global $user;
  if ($user->uid) {
    $userID = $user->uid;
  } else {
    $userID = NULL;
  }
  uc_gift_cards_create_transaction($card_id, 0, $amount, $code, $notes);
  watchdog('uc_gift_cards', 'Gift Card ID '.$card_id.' balance changed.');
  drupal_set_message('New Balance set to $' . $new_balance);
  return;
}

/**
 * Form Builder Function for Reset PIN form
 */
function uc_gift_cards_reset_pin_form($form, &$form_state, $gift_card) {
  $pin = $gift_card->getPin();
  $card_id = $gift_card->getID();
  $form['pin_reset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Reset PIN'),
    '#description' => t('This form allows you to reset (cancel) the PIN on this Gift Card. NOTE the owner will need to set a new one.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['pin_reset']['info'] = array(
    '#type' => 'item',
    '#title' => t('NOTE!'),
    '#markup' => 'This will erase the current PIN for this Gift Card! The card owner will need to reactivate this card to set a new one!',
  );
  if (!$gift_card->isPinSet()) {
    $form['pin_reset']['info']['#markup'] = 'The PIN on this Gift Card is not currently activated.';
    return $form;
  }
  $form['pin_reset']['gift_card_id'] = array(
    '#type' => 'hidden',
    '#value' => $gift_card->getID(),
  );
  $form['pin_reset']['submit'] =array(
    '#type' => 'submit',
    '#value' => t('Reset PIN'),
    '#submit' => array('uc_gift_cards_reset_pin_callback'),
    '#ajax' => array('wrapper' => 'uc-gift-cards-reset-pin-form')
  );
  return $form;
}

/**
 * Callback function for PIN Reset
 */
function uc_gift_cards_reset_pin_callback($form, &$form_state) {
  $card_id = check_plain($form_state['input']['gift_card_id']);
  $dbh = pdoConnect();
  // Reset Gift Card PIN
  $stmt = $dbh->prepare('UPDATE gift_cards SET pin = :pin WHERE id = :id');
  $stmt->execute(array(
    ':pin' => 0, 
    ':id' => $card_id
    ));
  global $user;
  if ($user->uid) {
    $userID = $user->uid;
  } else {
    $userID = NULL;
  }
  uc_gift_cards_create_transaction($card_id, 0, 0, 'PIN', 'PIN reset by' . $user->name . '.');
  watchdog('uc_gift_cards', 'Gift Card ID '.$card_id.' PIN reset.');
  drupal_set_message('Gift Card PIN reset.');
  return;
}

/**
 * Form Builder Function for Set PIN Form
 */
function uc_gift_cards_set_pin_form($form, &$form_state, $gift_card) {
  $form['pin_set'] = array(
    '#type' => 'fieldset',
    '#title' => t('Set PIN'),
    '#description' => t('This form allows you to Set the PIN on this Gift Card.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['pin_set']['info'] = array(
    '#type' => 'item',
    '#title' => t('NOTE!'),
  );
  if ($gift_card->isPinSet()) {
    $form['pin_set']['info']['#markup'] = 'This Card already has a PIN! Setting it here will over write the current PIN.';
  } else {
    $form['pin_set']['info']['#title'] = t('Instructions:');
    $form['pin_set']['info']['#markup'] = 'Enter a PIN below to activate this Gift Card.';
  }
  $form['pin_set']['gift_card_id'] = array(
    '#type' => 'hidden',
    '#value' => $gift_card->getID(),
  );
  $form['pin_set']['pin'] = array(
    '#type' => 'textfield',
    '#title' => t('Set the PIN.'),
    '#description' => t('Enter a 4 digit PIN to use when checking out with this Gift Card.'),
    '#name' => 'pin',
    '#maxlength' => 4,
    '#size' => 8,
    '#required' => TRUE,
  );
  $form['pin_set']['submit'] =array(
    '#type' => 'submit',
    '#value' => t('Set PIN'),
    '#submit' => array('uc_gift_cards_set_pin_callback'),
    '#ajax' => array('wrapper' => 'uc-gift-cards-set-pin-form')
  );
  return $form;
}

/**
 * Callback function for Set Pin Form
 */
function uc_gift_cards_set_pin_callback($form, &$form_state) {
  $card_id = check_plain($form_state['input']['gift_card_id']);
  $pin = check_plain($form_state['input']['pin']);
  if (strlen($pin) < 4 || strlen($pin) > 4 || !is_numeric($pin)) {
    form_set_error('pin', 'The PIN must be 4 numbers.');
    return $form;
  }
  $pin_hash = bcrypt_hash($pin);
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('UPDATE gift_cards SET pin = :pin WHERE id = :id');
  $stmt->execute(array(':pin' => $pin_hash, ':id' => $card_id));
  global $user;
  if ($user->uid) {
    $userID = $user->uid;
    $notes = 'PIN set by ' . $user->name;
  } else {
    $userID = NULL;
    $notes = 'PIN set via reset form.';
  }
  uc_gift_cards_create_transaction($card_id, 0, 0, 'PIN', $notes);
  watchdog('uc_gift_cards', 'Gift Card ID '.$card_id.' PIN set.');
  drupal_set_message('Gift Card PIN set.');
  return;
}

/**
 * Admin Settings Form Callback
 */
function uc_gift_cards_settings_form() {
  $site_name_description = t('<br>Site name used to associate user ids and gift card usage to specific sites. This is defined in config.inc and should not be changed once set.');
  $form['uc_gift_cards_site'] = array(
    '#type' => 'item',
    '#title' => t('Site Name'),
    '#markup' => UC_GIFT_CARDS_SITE_NAME . $site_name_description,
  );
  $form['uc_gift_cards_code_length'] = array(
    '#type' => 'item',
    '#title' => t('Gift Card Code Length'),
    '#markup' => UC_GIFT_CARDS_CODE_LENGTH . t('<br>The length of the Gift Card Code as defined in the file uc_gift_cards.config.inc')
  );
  $form['email_message'] = array(
    '#type' => 'text_format',
    '#title' => t('Recipient\'s Email Message'),
    '#default_value' => variable_get('uc_gift_cards_email_code_message', 'Set your message'),
    '#description' => t('This is the message text that will be sent to the recipient of a gift card, which includes the code and activation instructions.'),
    '#wysiwyg' => TRUE,
  );
  $form['email_purchaser'] = array(
    '#type' => 'text_format',
    '#title' => t('Email Message to Purchaser'),
    '#default_value' => variable_get('uc_gift_cards_email_code_purchaser', 'Enter a custom message here'),
    '#description' => t('This is the message that is sent to the purchaser ONLY if no recipient email was entered. The Gift Card code will be sent here instead.'),
    '#wysiwyg' => TRUE,
  );
    
  if (module_exists('token')) {
    $form['email_purchaser']['#description'] .= ' ' . t('You may use any of the following replacement patterns.');
    $form['email_purchaser']['#suffix'] = theme('token_tree', array('token_types' => array('uc_gift_cards')));
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Changes'),
    '#submit' => array('uc_gift_cards_settings_form_callback'),
    '#ajax' => array('wrapper' => 'uc-gift-cards-settings-form')
  ); 
  return $form;  
}

/**
 * Form handler for the Settings Form
 */
function uc_gift_cards_settings_form_callback($form, &$form_state) {
  $recipients_message = check_markup($form_state['input']['email_message']['value'], $form_state['input']['email_message']['format']);
  $purchaser_message =  check_markup($form_state['input']['email_purchaser']['value'], $form_state['input']['email_purchaser']['format']);
  variable_set('uc_gift_cards_email_code_message', $recipients_message);
  variable_set('uc_gift_cards_email_code_purchaser', $purchaser_message);
  drupal_set_message('Settings Saved');
  return $form;
}

/**
 * Callback function to display a user's gift cards
 */
function uc_gift_cards_user_view() {
  global $user;
  if (!$user) { 
    return; 
  }
  $user_email = $user->mail;
  $user_id = $user->uid;
  $markup = '';
  $site_name = UC_GIFT_CARDS_SITE_NAME;
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('SELECT * FROM users WHERE email = :email AND site = :site');
  $stmt->execute(array(
    ':email' => $user_email,
    ':site' => $site_name
    ));
  $gift_card_user = $stmt->fetch(PDO::FETCH_ASSOC);
  if ($gift_card_user) {
    /*** Get Gift Cards this user Owns and can use if there was a result ***/
    //$stmt = $dbh->prepare('SELECT * FROM gift_cards WHERE owner = :owner');
    $stmt = $dbh->prepare('SELECT * FROM gift_cards WHERE owner LIKE :owner AND active = 1');
    $stmt->execute(array(
      ':owner' => '%' . $gift_card_user['id'] . '%'
    ));
    $gift_cards_owned = $stmt->fetchAll(PDO::FETCH_CLASS, 'GiftCard');
    //dpm($gift_cards_owned); 
    /* We now have an array of gift_card objects that may or may not belong to this user.
     *  Now look at the owner array and see if they are an exact match or not. */
    foreach ($gift_cards_owned as $key => $card) {
      //dpm($card); 
      $owners = $card->getOwner();
      if (!in_array($gift_card_user['id'], $owners)) {
        unset($gift_cards_owned[$key]);
      }
     }
    } else {
    $gift_cards_owned = array();
  }
  // Create a table for our Owned Cards display
  $gco_table['header'] = array('Code', 'Balance', 'Activated?', 'Purchase Amount');
  $gco_table['rows'] = array();
  $gco_table['attributes'] = array();
  $gco_table['caption'] = 'Gift Cards Owned';   
  $gco_table['colgroups'] = array();
  $gco_table['sticky'] = TRUE;
  $gco_table['empty'] = 'No Gift Cards Were Found!';  
  foreach ($gift_cards_owned as $card) {
    if ($card->isPinSet() == TRUE) {
      $activated = 'Yes';
    } else {
      $activated = '<a href="/gift-cards/activate?gift_code=' . $card->getCode() . '" title="Activate This Card">No</a>';
    }
    $code = '<a href="/user/' . $user_id . '/gift_cards/' . $card->getID() . '">' . $card->getCode() . '</a>';
    $gco_table['rows'][] = array(
      $code,
      '$' . $card->getBalance(),      
      $activated,
      '$' . $card->getPurchaseAmount(),
    );
  }
  $markup .= theme_table($gco_table);  
  /** Get any Gift Cards this user has Purchased but have no Owner ***/
  $stmt = $dbh->prepare('SELECT * FROM gift_cards WHERE owner = :owner AND purchaser = :purchaser ');
  $stmt->execute(array(
    ':owner' => '[0]',
    ':purchaser' => $gift_card_user['id']
  ));
  $gift_cards_purchased = $stmt->fetchAll(PDO::FETCH_CLASS, 'GiftCard');
  //dpm($gift_cards_purchased);
  // Create a table for our Purchased Cards display
  $gcp_table['header'] = array('Code', 'Purchase Amount', 'Order Number', 'Purchase Date');
  $gcp_table['rows'] = array();
  $gcp_table['attributes'] = array();
  $gcp_table['caption'] = 'Gift Cards YOU have purchased that have no registered owner.';   
  $gcp_table['colgroups'] = array();
  $gcp_table['sticky'] = TRUE;
  $gcp_table['empty'] = 'No Gift Cards Were Found';  
  foreach ($gift_cards_purchased as $card) {
    $gcp_table['rows'][] = array(
      $card->getCode(),
      '$' . $card->getPurchaseAmount(),      
      $card->getOrderNumber(),
      date("m-d-y", $card->getPurchaseDate()),
    );
  }
  $markup .= theme_table($gcp_table); 
    $add_card_form = drupal_get_form('uc_gift_cards_add_card_user_form', $user->uid, $user->mail);
  $markup .= drupal_render($add_card_form);
  
  $content['raw_markup'] = array(
    '#type' => 'markup',
    '#markup' => $markup
  );
  return $content;
}

/**
 * Callback function to display a user's gift card transactions
 * 
 * @param $account
 *  The user object
 * 
 * @param $id
 *  This is the gift_cards.id of the card we want to see
 */
function uc_gift_cards_user_transactions($account, $id) {
  $markup = '';
  // Get our GiftCard
  $gift_card = uc_gift_cards_get_card_by_id($id);
  // Get transactions
  $transactions = uc_gift_cards_get_transactions_by_id($gift_card->getID());
  //dpm($gift_card->getOwner());
  if ($gift_card->isPinSet()) { $gc_set = "Yes"; } else { $gc_set = "No"; }
  $markup .= '<div class="gc-code"><h3>Gift Card Code <span class="gc-code-span">' . $gift_card->getCode() . '</span></h3>';
  $markup .= '<div class="gc-code-details"><span class="gc-details-label">Balance:</span> $' . $gift_card->getBalance() . '</div>';
  $markup .= '<div class="gc-code-details"><span class="gc-details-label">Activated?</span> ' . $gc_set . '</div>';

  // Create a table for our Transactions display
  $trans_table['header'] = array('Order Number', 'Site', 'Amount', 'Trans Type', 'Date', 'Notes');
  $trans_table['rows'] = array();
  $trans_table['attributes'] = array('id' => 'uc-gift-cards-transactions');
  $trans_table['caption'] = t('Gift Card Transactions for Gift Card ' . $gift_card->getCode());   
  $trans_table['colgroups'] = array();
  $trans_table['sticky'] = TRUE;
  $trans_table['empty'] = 'No Transactions Were Found For This Gift Card.'; 
  
  foreach ($transactions as $row) {
    /** @todo Set User and order number to a link if it's on the same site 
     * if (UC_GIFT_CARDS_SITE_NAME == $row['site']) then make link
     */
    $row['order_number'] == '0' ? $order_number = t('N/A') : $order_number = $row['order_number'];
    $trans_table['rows'][] = array(
      $order_number,
      $row['site'],
      $row['amount'],
      $row['code'],
      date("m-d-y \@ h:i A", $row['timestamp']),
      $row['notes']
      );
  }
  $markup .= theme_table($trans_table);
  $pin_set_form = drupal_get_form('uc_gift_cards_set_pin_form', $gift_card);
  $markup .= '<div class="gc-operations"><h2>Card Operations</h2>';
  $markup .= drupal_render($pin_set_form);     
  if ($gift_card->getBalance() <= 0) {
    $delete_card_form = drupal_get_form('uc_gift_cards_delete_card_form', $gift_card->getID());
    $markup .= drupal_render($delete_card_form);
  }
  $content['raw_markup'] = array(
    '#type' => 'markup',
    '#markup' => $markup
  );
  return $content;
}

/**
 * Form Builder Function for the Archive Card form
 */
function uc_gift_cards_archive_card_form($form, &$form_state, $gift_card) {
  $form['archive'] = array(
    '#type' => 'fieldset',
    '#title' => t('Set Status: Active or Inactive'),
    '#description' => t('This form allows you to set the status of a gift card as \'Active\' or \'Inactive\'.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['archive']['active'] = array(
    '#type' => 'radios',
    '#title' => t('Inactive or Active'),
    '#default_value' => $gift_card->getActive(),
    '#options' => array(0 => 'Inactive', 1 => 'Active'),
    '#description' => t('Choose the status of this gift card.')
  );
  $form['archive']['gift_card_id'] = array(
    '#type' => 'hidden',
    '#value' => $gift_card->getID(),
  );
  $form['archive']['submit'] =array(
    '#type' => 'submit',
    '#value' => t('Set Status'),
    '#submit' => array('uc_gift_cards_archive_card_callback'),
    '#ajax' => array('wrapper' => 'uc-gift-cards-archive-card-form')
  );  
  if ($gift_card->getBalance() > 0) {
    $form['archive']['warning'] = array(
      '#type' => 'item',
      '#title' => t('NOTE!'),
      '#markup' => t('This card still has a balance, and setting it to Inactive status will remove it from the customer\'s view!')
    );
  }
  return $form;
}

/**
 * Handles the archive and delete card forms
 */
function uc_gift_cards_archive_card_callback($form, &$form_state) {
  global $user;
  $active = $form_state['input']['active'];
  $gift_card_id = $form_state['input']['gift_card_id'];
  // Update the card's active state
  uc_gift_cards_update_active($gift_card_id, $active);
  // Record action to transactions
  if ($active == '1') {
      $status = t('Active');
    } else {
    $status = t('Inactive');
  }
  uc_gift_cards_create_transaction($gift_card_id, 0, 0, 'Status', "Changed to $status by $user->name" );
  drupal_set_message("Status changed to $status.");
  return $form;
}

/**
 * Form Builder Function for the Delete Card form
 *  This is presented to the owner/customer of the gift card once their balance is zero.
 * It allows them to 'delete' the card, which basically just removes it from the view.
 * The card and its transactions are not deleted and are still accessible by store admins.
 * Note that this uses the same callback function as the admin 'archive' form
 * 
 * @param $id
 *  The Gift Card id
 */
function uc_gift_cards_delete_card_form($form, &$form_state, $id) {
  $form['archive'] = array(
    '#type' => 'fieldset',
    '#title' => t('Delete this Gift Card'),
    '#description' => t('This allows you to delete this card now that it has been used.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['archive']['active'] = array(
    '#type' => 'hidden',
    '#value' => 0,
  );
  $form['archive']['gift_card_id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['archive']['warning'] = array(
      '#type' => 'item',
      '#title' => t('NOTE!'),
      '#markup' => t('By deleting this card, you will no longer have access to it\'s transaction records!')
    );
  $form['archive']['submit'] =array(
    '#type' => 'submit',
    '#value' => t('Delete Card'),
    '#submit' => array('uc_gift_cards_archive_card_callback'),
    '#ajax' => array('wrapper' => 'uc-gift-cards-delete-card-form')
  );  
  return $form;
}

/**
 * Form Builder Function
 *  This form allows an authenticated user to add an EXISTING and ACTIVATED card from a different 
 * web site to this account on this site, so they can associate with this user and see transactions and details.
 * @param $uid
 *  The authenticated user id
 */
function uc_gift_cards_add_card_user_form($form, &$form_state, $uid = NULL, $email = NULL) {
  if ($uid === NULL) {
    return $form;
  }
  $description = 'This allows you to add an existing card from another Belvedere Designs web site to this site and user account.<br>'
      . 'Note that the Gift Card must already be activated with a valid PIN.<br>If you just need to activate a new Gift Card Code,'
      . ' then see our <a href="/gift-cards/activate/">Card Activation</a> page.';
  $form['add_card_user'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add Existing Gift Card'),
    '#description' => t($description),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['add_card_user']['gift_code'] = array(
    '#type' => 'textfield',
    '#title' => t('Gift Card Code'),
    '#description' => t('Enter the Gift Card Code you want to associate with this account.'),
    '#name' => 'gift_code',
    '#maxlength' => UC_GIFT_CARDS_CODE_LENGTH,
    '#size' => UC_GIFT_CARDS_CODE_LENGTH * 2,
    '#required' => TRUE
      );
  $form['add_card_user']['pin'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter the PIN.'),
    '#description' => t('Enter a 4 digit PIN to use when you checkout with this code.'),
    '#name' => 'pin',
    '#maxlength' => 4,
    '#size' => 8,
    '#required' => TRUE,
  );
  $form['add_card_user']['uid'] = array(
    '#type' => 'hidden',
    '#value' => $uid,
  );
  $form['add_card_user']['email'] = array(
    '#type' => 'hidden',
    '#value' => $email,
  );
  $form['add_card_user']['submit'] =array(
    '#type' => 'submit',
    '#value' => t('ADD IT!'),
    '#submit' => array('uc_gift_cards_add_card_user_form_callback'),
    '#ajax' => array('wrapper' => 'uc_gift_cards_add_card_user_form'),
  );
  return $form;
}

/**
 * Allows a user to add an existing and activated Gift Card to their account
 */
function uc_gift_cards_add_card_user_form_callback($form, &$form_state){
  // We will sanitize user input of course, but we're not really going to do much validation.
  // If they don't know the code or PIN... well we really can't help them with that here.
  $gift_code = check_plain(strtoupper($form_state['input']['gift_code']));
  $pin = check_plain($form_state['input']['pin']);
  $uid = check_plain($form_state['input']['uid']);
  $email = check_plain($form_state['input']['email']);
  $gift_card = uc_gift_cards_get_from_db($gift_code);
  if ( !$gift_card->isPinSet() ) {
    /** @todo check owner email for security? **/
    form_set_error('pin', t('This card has not been activated. You need to <a href="/gift-cards/activate/">Activate</a> it.'));
    return $form;
  }
  if ($gift_card === FALSE || $gift_card->validatePin($pin) === FALSE) {
    form_set_error('gift_code', t('Please enter a valid code and PIN.'));
    $form_state['rebuild'] = TRUE;
    $form_state['complete form']['add_card_user']['#collapsed'] = TRUE;
    watchdog('uc_gift_cards', 'User failed at attempting to add a Gift Card to their account.');
    return $form_state;
  }
  // We have a valid card with a PIN. Check the owners - just add this user to the user array...?
  // First see if this site user has a gift card user entry
  $site = UC_GIFT_CARDS_SITE_NAME;
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('SELECT * FROM users WHERE uid = :uid AND site = :site');
  $stmt->execute(array(
    ':uid' => $uid,
    ':site' => $site,
  ));
  $gift_card_user = $stmt->fetch(PDO::FETCH_ASSOC);

  // No result so check with the email
  if (!$gift_card_user) {
  $stmt = $dbh->prepare('SELECT * FROM users WHERE email = :email AND site = :site');
  $stmt->execute(array(
    ':email' => $email,
    ':site' => $site,
  ));
  $gift_card_user = $stmt->fetch(PDO::FETCH_ASSOC);    
  }
  
  // If still no result, then we need to create a new gift card user
  if (!$gift_card_user) {
    $gift_card_user = uc_gift_cards_add_user($site, $uid, $email);
  }
  
  // Now we can just add this user to the owner array
  $owners = $gift_card->getOwner();
  array_push($owners, $gift_card_user['id']);
  // Now update the database and current page?
  $stmt = $dbh->prepare('UPDATE gift_cards SET owner = :owner WHERE id = :id');
  $stmt->execute(array(
    ':owner' => json_encode($owners),
    ':id' => $gift_card->getID(),
  ));
  drupal_set_message("Gift Card <strong>$gift_code</strong> added to your account.");
  return $form;
}

/**
 * Form builder to allow the actual deletion of a card.
 * This will only delete the card code from gift_cards, but will leave the transaction records.
 * 
 * @param $id
 * The gift_card.id
 */
function uc_gift_cards_admin_delete_card_form($form, &$form_state, $id) {
  drupal_add_js('(function($){ $(function(){$("input[name=\'delete-gift-card\']").click(function(){return confirm("Are you sure you want to delete this Gift Card?")});}); })(jQuery);', array('type' => 'inline'));
  $form['delete'] = array(
    '#type' => 'fieldset',
    '#title' => t('Delete this Gift Card'),
    '#description' => t('This allows you to delete this card from the database. Note that transaction records will not be deleted.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['delete']['info'] = array(
    '#type' => 'item',
    '#title' => t('Note:'),
    '#markup' => t('The card balance must be zero, and you must have permission to delete Gift Cards.')
  );
  $form['delete']['gift_card_id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['delete']['submit'] =array(
    '#type' => 'submit',
    '#value' => t('Delete Card'),
    '#name' => 'delete-gift-card',
    '#submit' => array('uc_gift_cards_admin_delete_card_callback'),
  );  
  return $form;
}

/**
 * Callback function that deletes a gift card from the database.
 * Note that transactions are not deleted. This form should only be displayed
 * to trusted users. 
 *  - User must have permission to delete Gift Cards
 *  - Gift Card balance must be 0
 */
function uc_gift_cards_admin_delete_card_callback($form, &$form_state) {
   if (!user_access('delete gift cards')) {
     drupal_set_message('You don\'t have permission to delete Gift Cards.', 'error');
     return $form;
   }
   $gift_card_id = $form_state['input']['gift_card_id'];
   $gift_card = uc_gift_cards_get_card_by_id($gift_card_id);
   if ($gift_card->getBalance() > 0) {
     drupal_set_message('This Gift Card still has a balance on it! The balance must be zero before you can delete it.', 'error');
     return $form;
   }
  $code = $gift_card->getCode();
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('DELETE FROM gift_cards WHERE id = :id');
  $stmt->execute(array(
    ':id' => $gift_card_id,
  ));
  global $user;
  uc_gift_cards_create_transaction($gift_card_id, 0, 0, 'Deleted', "Gift Card Code $code deleted by $user->name");
  watchdog('uc_gift_cards', "Gift Card $code with an id of $gift_card_id deleted by user $user->name");
  drupal_set_message(t("Gift Card $code deleted."));
  drupal_goto('/admin/store/gift_cards/');
}