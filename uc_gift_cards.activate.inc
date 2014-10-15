<?php

/** 
 * @file Activate Gift Cards Page
 */

/**
 * Callback for the Activate Gift Card Menu Hook
 */
function _uc_gift_cards_activate() {
  $markup = '';
  // First check if user is logged in
  global $user;
  // Grab our form
  $form = drupal_get_form('uc_gift_cards_activate_form');

  if ($user->uid) { 
    $markup .= '<p>This will activate and associate a Gift Card with your user <a href="/user">account</a>.</p>';
  } else {
    $markup .= '</p>Please <a href="/user">log in</a> to your account (or <a href="/user">register</a> a new one) to associate a Gift Card with your account, '
        . 'or continue to activate your Gift Card without associating it with a user account.</p>';
  }  
  $markup .= drupal_render($form);
  
  $content['raw_markup'] = array(
    '#type' => 'markup',
    '#markup' => $markup
  );
  return $content;
}

/**
 * Callback form
 */
function uc_gift_cards_activate_form($form, &$form_state) {
  if (isset($_REQUEST['gift_code'])) {
    $gift_code = check_plain($_REQUEST['gift_code']);
  } else {
    $gift_code = '';
  }
  global $user;
  if (isset($user->mail)) {
    $email = $user->mail;
  } else {
    $email = '';
  }
  $form['gift_code'] = array(
    '#type' => 'textfield',
    '#title' => t('Gift Card Code'),
    '#description' => t('Enter the Gift Code you need to activate. This is the Gift Card code you received in your email.'),
    '#name' => 'gift_code',
    '#default_value' => $gift_code,
    '#maxlength' => UC_GIFT_CARDS_CODE_LENGTH,
    '#size' => UC_GIFT_CARDS_CODE_LENGTH * 2,
    '#required' => TRUE
      );
  $form['pin'] = array(
    '#type' => 'textfield',
    '#title' => t('Choose a PIN'),
    '#description' => t('Choose a 4 digit PIN to use when you checkout with your Gift Card.'),
    '#name' => 'pin',
    '#maxlength' => 4,
    '#size' => 8,
    '#required' => TRUE,
  );
  $form['email'] = array(
    '#type' => 'textfield',
    '#title' => t('Email Address'),
    '#description' => t('If you recieved the Gift Card as a gift through email, enter your email address here for verification.'),
    '#name' => 'email',
    '#default_value' => $email,
    '#size' => 36,
  );
  $form['submit'] =array(
    '#type' => 'submit',
    '#value' => t('ACTIVATE!'),
    '#submit' => array('uc_gift_cards_activate_code'),
    '#ajax' => array('wrapper' => 'uc-gift-cards-activate-form')
  );
  return $form;
}

/** 
 * Callback to activate a Gift Card Code
 */
function uc_gift_cards_activate_code(&$form, &$form_state) {
  // Get our form values
  $code = strtoupper(check_plain($form_state['input']['gift_code']));
  $pin = check_plain($form_state['input']['pin']);
  $email = check_plain($form_state['input']['email']);
  // Validation
  if (strlen($code) != UC_GIFT_CARDS_CODE_LENGTH) {
    form_set_error('gift_code', 'Please enter your Gift Code');
    $form_state['rebuild'] = TRUE;
    return $form;
  }
  if (strlen($pin) < 4 || strlen($pin) > 4 || !is_numeric($pin)) {
    form_set_error('pin', 'Your PIN must be 4 numbers.');
    $form_state['rebuild'] = TRUE;
    return $form;
  }
  // Validate email 
  if (!empty($email) && !valid_email_address($email)) {
    form_set_error('email', t('Please enter a valid email address!'));
    $form_state['rebuild'] = TRUE;
    return $form;  
  }
  // Now check the DB for this Gift Card code
  $gift_card = uc_gift_cards_get_from_db($code);
  if ($gift_card == FALSE) { 
    form_set_error('gift_code', 'Gift Card Code not found.');
    $form_state['rebuild'] = TRUE;
    return $form;
  }
  // Check the length of pin from DB to see if its already set
  if (strlen($gift_card->getPin()) == 60) {
    form_set_error('gift_code', 'This card has already been activated!');
    $form_state['rebuild'] = TRUE;
    return $form;
  }
  // Lastly, we need to check the email address if appropriate
  $owner_email = $gift_card->getOwnerEmail();
  //dpm($owner_email);
  if ($owner_email && !in_array($email, $owner_email)) {
    form_set_error('email', 'Email address does not match the email address this code was sent to!');
    $form_state['rebuild'] = TRUE;
    return $form;
  }
   
  // Passed all tests - save the pin to DB
  $pin_hash = bcrypt_hash($pin);
  uc_gift_card_save_pin($code, $pin_hash);
  
  //Save or update owner info if available (Annon user is uid==0)
  global $user;
  $userID = $user->uid;
  $site = UC_GIFT_CARDS_SITE_NAME;
  
  /* The user is logged in OR provided an email address, so log/update this info */
  if (user_is_logged_in() || $email) {
    // First we need to get this gift_cards user id - or FALSE if they don't exist yet in the users table
    if (user_is_logged_in()) {
      $gift_cards_user_id = uc_gift_cards_does_user_exist($site, $userID, $user->mail);
    } else {
      // They are not logged in, so we have to fall back on the email address provided
      $gift_cards_user_id = uc_gift_cards_does_user_exist($site, FALSE, $email);     
    }
    // This will be our array of current owner ids, if any
    $owners = $gift_card->getOwner();
    // $registered_owner is either True or False if this user id is in the owners array
    if ($gift_cards_user_id) {
      (in_array($gift_cards_user_id, $owners)) ? $registered_owner = TRUE : $registered_owner = FALSE;
    } else {
      $registered_owner = FALSE;
    }
    
  /* Test 1: User is logged in, has a users.id, and is a registered owner of this card */   
    if (user_is_logged_in() && $gift_cards_user_id && $registered_owner) {
      // The logged in user is the registered owner - update them
      uc_gift_cards_update_user($gift_cards_user_id, $site, $userID, $user->mail);
    } 
  /* Test 1a: User is logged in, has a users.id, but is NOT a registered owner of this card */
    elseif (user_is_logged_in() && $gift_cards_user_id && !$registered_owner) {
      // Update the user's info
      uc_gift_cards_update_user($gift_cards_user_id, $site, $userID, $user->mail);
      // And assign them to the owners array
      array_push($owners, $gift_cards_user_id);
      $gift_card->setOwnerSimple(json_encode($owners));
      // Update database with new owners
      uc_gift_cards_save_owner($gift_card->getID(), $owners);
      // Create a transaction record
      uc_gift_cards_create_transaction($gift_card->getID(), 0, 0, 'Owner', "New owner $user->mail added to card.");
    }
  /* Test 2: User is logged in, no users.id - can't be a registered owner */  
      elseif (user_is_logged_in() && !$gift_cards_user_id) {
        // Create a New Gift Cards User
        $gift_cards_user_id = uc_gift_cards_add_user($site, $userID, $user->mail);
        // And assign them to the owners array
        array_push($owners, $gift_cards_user_id);
        $gift_card->setOwnerSimple(json_encode($owners));
        // Update database with new owners
        uc_gift_cards_save_owner($gift_card->getID(), $owners);
        // Create a transaction record
        uc_gift_cards_create_transaction($gift_card->getID(), 0, 0, 'Owner', "New owner $user->mail added to card.");
      }     
  /* Test 3: User not logged in, provided email, has a users.id - is registered owner */
      elseif (user_is_anonymous() && $email && $gift_cards_user_id && $registered_owner) {
        // Do nothing?  
     } 
  /* Test 4: User not logged in, provided email - no users.id and so can't be a registered owner */  
     elseif (user_is_anonymous() && $email && !$gift_cards_user_id) {
       // Create a New Gift Cards User
       $gift_cards_user_id = uc_gift_cards_add_user($site, NULL, $email);
       // And assign them to the owners array
       array_push($owners, $gift_cards_user_id);
       $gift_card->setOwnerSimple(json_encode($owners));
       // Update database with new owners
       uc_gift_cards_save_owner($gift_card->getID(), $owners);
       // Create a transaction record
       uc_gift_cards_create_transaction($gift_card->getID(), 0, 0, 'Owner', "New owner $email added to gift card.");
    }
  }
  
  // Add note to transactions
  uc_gift_cards_create_transaction($gift_card->getID(), 0, 0, 'PIN', 'Card Activated with new PIN.');
  // Log to watchdog
  watchdog('uc_gift_cards', 'Gift Card ID ' . $gift_card->getCode() . ' activated by $user->uid: .' . $userID);
  // Set message for the user
  drupal_set_message(t('Gift Card <strong>') . $code . t('</strong> Activated! You may now use it during checkout.'), 'status');
  return $form;
}
