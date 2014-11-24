<?php

/**
 * @file
 * Database connections and functions
 */

// Database Config File
require_once 'uc_gift_cards_db.config.inc';

/**
 * Function to handle our uc_gift_cards database connection
 * 
 * @return PDO connection
 */
function pdoConnect() {
    $dbh = new PDO('mysql:host='.UC_GIFT_CARDS_DB_HOST.';dbname='.UC_GIFT_CARDS_DB_NAME, UC_GIFT_CARDS_DB_USER, UC_GIFT_CARDS_DB_PASS);
    $dbh->setAttribute( PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    $dbh->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);
    return $dbh;
  }
  
/**
 * Function to get a gift card object from the database
 * 
 * @param $code
 *  The Gift Card Code
 * 
 * @return gift card object or FALSE
 */
function uc_gift_cards_get_from_db($code) {
  // connect to DB
  $dbh = pdoConnect();
  // Prepare our select statement
  $stmt = $dbh->prepare('SELECT * FROM gift_cards WHERE code = :code');
  $stmt->execute(array(':code' => $code));
  // Run our query
  $gift_card = $stmt->fetchObject('GiftCard');
  // Returns the gift card object or FALSE
  return $gift_card;
}

/**
 * Save a PIN to the DB
 * @param type $code
 *  The gift card code
 * @param type $pin
 *  The gift card pin
 */
function uc_gift_card_save_pin($code, $pin) {
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('UPDATE gift_cards SET pin = :pin WHERE code = :code');
  $stmt->execute(array(':pin' => $pin, ':code' => $code));
}

/**
 * Update a user record
 * @param $id
 *  The users.id 
 * @param $site
 * @param $uid
 *  The users.uid which is the global $user->uid for a particular site
 * @param $email
 */
function uc_gift_cards_update_user($id, $site, $uid, $email) {
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('UPDATE users SET email = :email, uid = :uid, site = :site WHERE id = :id');
  $stmt->execute(array(
    ':email' => $email,
    ':uid' => $uid,
    ':site' => $site,
    ':id' => $id,
  ));
}

/**
 * Add a new user to the users table
 * @param $site
 * @param $uid
 *  The global $user->uid, which may be NULL
 * @param $email
 * @return $id
 *  The new users.id
 */
function uc_gift_cards_add_user($site, $uid, $email) {
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('INSERT INTO users (site, uid, email) VALUES (:site, :uid, :email)');
  $stmt->execute(array(
    ':site' => $site,
    ':uid' => $uid,
    ':email' => $email,
      ));  
  $id = $dbh->lastInsertId();
  return $id;
}

/**
 * Checks to see if a user is present in our uc_gift_cards users database
 * 
 * @param $site
 * @param $uid
 *  global $user->uid
 * @param $email
 *  global $user->mail
 * @return $user.id or FALSE
 */
function uc_gift_cards_does_user_exist($site, $uid, $email) {
  // Check for the $uid first
  $dbh = pdoConnect();
  if ($uid) {
    $stmt = $dbh->prepare('SELECT * FROM users WHERE site = :site AND uid = :uid');
    $stmt->execute(array(
      ':site' => $site,
      ':uid' => $uid,
    ));
    $gift_card_user = $stmt->fetch(PDO::FETCH_ASSOC);
    if ($gift_card_user) {
      return $gift_card_user['id'];
    }
  }
  $stmt = $dbh->prepare('SELECT * FROM users WHERE site = :site AND email = :email');
  $stmt->execute(array(
    ':site' => $site,
    ':email' => $email,
  )); 
  $gift_card_user = $stmt->fetch(PDO::FETCH_ASSOC);
  if ($gift_card_user) {
    return $gift_card_user['id'];
  }
  return FALSE;
}
  
/**
 * Crypt functions for pin from
 * https://gist.github.com/marcoarment/1053158
 * @returns 60 char hashed and salted version of the pin
 */  
function bcrypt_hash($password, $work_factor = 8)
{
  if (version_compare(PHP_VERSION, '5.3') < 0) throw new Exception('Bcrypt requires PHP 5.3 or above');

  if (! function_exists('openssl_random_pseudo_bytes')) {
      throw new Exception('Bcrypt requires openssl PHP extension');
  }

  if ($work_factor < 4 || $work_factor > 31) $work_factor = 8;
  $salt = 
      '$2a$' . str_pad($work_factor, 2, '0', STR_PAD_LEFT) . '$' .
      substr(
          strtr(base64_encode(openssl_random_pseudo_bytes(16)), '+', '.'), 
          0, 22
      )
  ;
  return crypt($password, $salt);
}

/**
 * Verifies that an entered password (or code) is valid when compared to an encrypted value
 * @param string $password
 *  The password that needs to be checked
 * @param string $stored_hash
 *   The stored value to check against
 * @param type $legacy_handler
 * @return TRUE or FALSE
 * @throws Exception
 */
function bcrypt_check($password, $stored_hash, $legacy_handler = NULL)
{
  if (version_compare(PHP_VERSION, '5.3') < 0) throw new Exception('Bcrypt requires PHP 5.3 or above');

  if (bcrypt_is_legacy_hash($stored_hash)) {
      if ($legacy_handler) return call_user_func($legacy_handler, $password, $stored_hash);
      else throw new Exception('Unsupported hash format');
  }

  return crypt($password, $stored_hash) == $stored_hash;
}

function bcrypt_is_legacy_hash($hash) { return substr($hash, 0, 4) != '$2a$'; }

/**
 * Gets transactions from the db for a specified gift card id
 * @param $id
 *  The gift_card id
 * @return Assoc array of transaction records for the gift_card
 */
function uc_gift_cards_get_transactions_by_id($id) {
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('SELECT * FROM transactions WHERE card_id = :id');
  $stmt->execute(array(':id' => $id));
  $transactions = $stmt->fetchAll(PDO::FETCH_ASSOC);
  return $transactions;
}

/**
 * Gets a GiftCard object from the db by its id
 * @param type $id
 *  The gift_card id
 * @return the GiftCard Object
 */
function uc_gift_cards_get_card_by_id($id) {
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('SELECT * FROM gift_cards WHERE id = :id');
  $stmt->execute(array(':id' => $id));
  $gift_card = $stmt->fetchObject('GiftCard');
  return $gift_card;
}

/**
 * Gets a GiftCard Object from the db by its code
 * @param $code
 *  The gift_card code
 * @return the GiftCard object
 */
function uc_gift_cards_get_card_by_code($code) {
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('SELECT * FROM gift_cards WHERE code = :code');
  $stmt->execute(array(':code' => $code));
  $gift_card = $stmt->fetchObject('GiftCard');
  return $gift_card;
}

/**
 * Returns the owner of a Gift Card
 * @param $gift_card
 *  The gift_card object
 * @return $owner
 *  Either the user name (as a link) of the registered user if owner is registered on this site, or
 * the email address if owner is registerd on a different site, or the text 'No registered owner'
 */
function uc_gift_cards_return_owner($gift_card) {
  $site = UC_GIFT_CARDS_SITE_NAME;
  $id_array = $gift_card->getOwner();
  $owner = NULL;
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('SELECT * FROM users WHERE id = :id');
  foreach ($id_array as $id) {
    $stmt->execute(array(
      ':id' => $id,
    ));
    $result = $stmt->fetchAll(PDO::FETCH_ASSOC);
    // If $result is empty and $owner has not yet been set, set $owner to No owner
    if (!$result && $owner === NULL) {
      $owner = t('No registered owner');
      continue;
    }
    // Next check results for this site
    if (isset($result[0]['site']) && $result[0]['site'] === $site) {
      // See if we have a uid
      if (isset($result[0]['uid'])) {
        // Yes, load this user object and return their name
        $user_account = user_load($result[0]['uid']);
        return '<a href="/users/' . $user_account->name . '">' . $user_account->name . '</a>';
      }
      // No 'uid' so see if we have an email
      if (isset($result[0]['email'])) {
        $owner = $result[0]['email'];
        return $owner;
      }
      // This shouldn't happen, but suppose it's possible
      if ($owner === NULL) {
        $owner = t('*No registered owner*');
      }
    }
    // Now check results for an email address - not the current site
    if (isset($result[0]['email'])) {
      // We have an email address, so set $owner to it - may get overwritten by checks above on subsequent ids
        $owner = $result[0]['email'];
    }
  }
  // We didn't get a hit for the current site, so return $owner
  return $owner;
}

/**
 * 
 * @param $id
 * The gift_card.id
 * @param $owners
 * The owner(s) of a gift card - ensures it's not an array still
 */
function uc_gift_cards_save_owner($id, $owner) {
  if (gettype($owner) == 'array') {
    $owner = json_encode($owner);
  }
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('UPDATE gift_cards SET owner = :owner WHERE id = :id');
  $stmt->execute(array(
    ':owner' => $owner,
    ':id' => $id,
  ));
}

/**
 * Updates a Gift Card's Active field 
 * @param $id
 *  The gift_card.id
 * @param $active
 *  Is either 1 for Active, or 0 for Inactive
 */
function uc_gift_cards_update_active($id, $active) {
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('UPDATE gift_cards SET active = :active WHERE id = :id');
  $stmt->execute(array(
    ':active' => $active,
    ':id' => $id,
  ));
}

/**
 * Creates a new transaction record
 * 
 * @param $id
 *  The gift card id
 * @param $order_number
 * @param $amount
 * @param $code
 *  The Code describing the transaction type (Debit, Credit, PIN, etc)
 * @param $notes
 */
function uc_gift_cards_create_transaction($id, $order_number, $amount, $code, $notes) {
  global $user;
  $dbh = pdoConnect();
  $stmt = $dbh->prepare('INSERT INTO transactions (card_id, order_number, uid, site, ip, amount, code, timestamp, notes) VALUES (:card_id, :order_number, :uid, :site, :ip, :amount, :code, :timestamp, :notes)');
  $stmt->execute(array(
    ':card_id' => $id, 
    ':order_number' => $order_number, 
    ':uid' => $user->uid, 
    ':site' => UC_GIFT_CARDS_SITE_NAME, 
    ':ip' => check_plain($_SERVER['REMOTE_ADDR']),
    ':amount' => $amount,
    ':code' => $code,
    ':timestamp' => time(),
    ':notes' => $notes,
      ));
}