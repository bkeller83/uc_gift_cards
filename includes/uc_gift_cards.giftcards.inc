<?php

/**
 * @file GiftCard Class
 */
class GiftCard {
  
  private $id; // auto-increment from DB - referenced in transactions
  private $code; // the generated card code
  private $pin; // user activated pin
  private $order_number; // order number **associated with purchase**
  private $purchase_amount; 
  private $purchase_date;
  private $site; // site purchased from
  private $balance;
  /** The gift_card->owner field will hold a json_decoded simple array of values which are users.id
   * The users table holds fields id, site, uid, email 
   * NOTES: uid is the global $user->uid for the site. 
   *        While uid will always remain the same, the user may change their email address. 
   *        A record in table users may have a site and email, but no uid if the customer is not logged in
   *          or registered on the site, as we don't require that to activate a gift card.
   *        Use json_encode / json_decode to store / retrieve gift_card->owner data in gift_cards.owner
   */
  public $owner; // A json_decoded array of Gift Cards users.id (not to be confused with global $user->uid)use json_encode/decode
  public $purchaser; // Gift Cards users.id
  public $active; // Boolean 1/0 (tinyint) for true/false - Tracks if this card is active or not, which gives us a way to archive used card codes
  
  public function setID($id) {
    $this->id = $id;
  }
  public function getID() {
    if ($this->id) { 
      return $this->id;
    } else {
      // We need to get the id based on the code and set it for this gift card
      $dbh = pdoConnect();
      $stmt = $dbh->prepare('SELECT id FROM gift_cards WHERE code = :code');
      $stmt->execute(array(
        ':code' => $this->getCode(),
          ));
      $results = $stmt->fetch();
      return $results['id'];
    }
  }
  public function genCode() {
  // Generate x random characters - leaving outvowels and 0, 1
  $chars = "23456789BCDFGHJKLMNPQRSTVWXYZ";
  $code = "";
  $dbh = pdoConnect();
  do {
    // Generate a code
    for ($i = 0; $i < UC_GIFT_CARDS_CODE_LENGTH; $i++) {
      $code .= $chars[mt_rand(0, strlen($chars)-1)];
    }
    $stmt = $dbh->prepare('SELECT code FROM gift_cards WHERE code = :new_code');
    $stmt->execute(array(':new_code' => $code));
    // See if it exists in the DB
    $result = $stmt->fetchAll(); 
  } while ( count($result) != 0 );
  // Return a unique code
  return $code;
  }
  public function setCode($code){
  $this->code = $code;
  }
  public function getCode() {
    return $this->code;
  }
  public function setPin($pin) {
    $this->pin = $pin;
  }
  public function getPin() {
    return $this->pin;
  }
  public function validatePin($pin) {
    $pin_hash = $this->getPin();
    if (bcrypt_check($pin, $pin_hash)) {
      return TRUE;
    }
    return FALSE;
  }
  public function isPinSet() {
    if (strlen($this->getPin()) > 50) {
      return TRUE;
    }
    return FALSE;
  }
  public function setOrderNumber($order_number) {
    $this->order_number = $order_number;
  }
  public function getOrderNumber() {
    return $this->order_number;
  }
  public function setPurchaseAmount($purchase_amount) {
    $this->purchase_amount = $purchase_amount;
  }
  public function getPurchaseAmount() {
    return $this->purchase_amount;
  }
  public function setPurchaseDate($date) {
    $this->purchase_date = $date;
  }
  public function getPurchaseDate() {
    return $this->purchase_date;
  }
  public function setSite() {
    $site = UC_GIFT_CARDS_SITE_NAME;
    if ($site == '') {
      drupal_set_message(t('Site Name is NOT set!', 'warning'));
      watchdog('uc_gift_cards', t('Site Name needs to be set!'), 'warning', '../admin/store/gift_cards/settings' );
    }
    $this->site = $site;
  }
  public function getSite() {
    return $this->site;
  }
  public function setBalance($balance) {
    $this->balance = $balance;
  }
  public function getBalance() {
    return $this->balance;
  }
  /**
   * Gets or creates users.id from table users
   */
  public function setPurchaser($array) {
    // array = ('uid' => $order->uid, 'email' => $order->primary_email);
    // Check to see if this uid or email is in users for this site
    $uid = $array['uid'];
    $email = $array['email'];
    $site = UC_GIFT_CARDS_SITE_NAME;
    $dbh = pdoConnect();
    $stmt = $dbh->prepare('SELECT * FROM users WHERE uid = :uid AND site = :site');
    $stmt->execute(array(':uid' => $uid, ':site' => $site));
    // See if it exists in the DB
    $result = $stmt->fetchAll(); 
    if (!empty($result)) {
      $purchaser = $result[0]['id'];
      $this->purchaser = $purchaser;
      return;
    }
    // No results found for uid, check this email address    
    $stmt = $dbh->prepare('SELECT * FROM users WHERE email = :email AND site = :site');
    $stmt->execute(array(':email' => $email, ':site' => $site));
    // See if it exists in the DB
    $result = $stmt->fetchAll(); 
    if (!empty($result)) {
      $purchaser = $result[0]['id'];
      $this->purchaser = $purchaser;
      return;
    }
    // No results for this uid or email address - create new user table row
    $stmt = $dbh->prepare('INSERT INTO users (site, uid, email) VALUES (:site, :uid, :email)');
    $stmt->execute(array(
      ':site' => $site,
      ':uid' => $uid,
      ':email' => $email,
        ));  
    $this->purchaser = $dbh->lastInsertId();
   
  }
  public function getPurchaser() {
    return $this->purchaser;
  }
  public function getPurchaserEmail() {
    $site = UC_GIFT_CARDS_SITE_NAME;
    $dbh = pdoConnect();
    $stmt = $dbh->prepare('SELECT * FROM users WHERE id = :id AND site = :site');
    $stmt->execute(array(':id' => $this->getPurchaser(), ':site' => $site));
    $result = $stmt->fetchAll();
    if ($result) {
      $email = $result[0]['email'];
      return $email;
    }
    return FALSE;
  }
  
  /**
   * CREATES a new owner record in users when we know we have all the correct info
   * 
   * @param type $site
   *  The site name
   * @param type $uid
   *  The global $user->uid for this site
   * @param type $email
   *  The email address from $user->mail
   * 
   * @return $owner
   *  The users.id from the newly created user record
   */
  public function setNewOwner($site, $uid, $email) {
    // Ensure there is not already a user for this site with this uid or email?
    
    $dbh = pdoConnect(); 
    $stmt = $dbh->prepare('INSERT INTO users (site, uid, email) VALUES (:site, :uid, :email)');
    $stmt->execute(array(
      ':site' => $site,
      ':uid' => $uid,
      ':email' => $email,
        ));  
    $owner = $dbh->lastInsertId();
    return $owner;
  }
  
  /**
   * Sets the owner when the Gift Card is initially created
   *  @param $attributes
   *    The array of product attributes
   *  $return array containing the initial owner as '0' => users.id
   */
  public function setOwner($attributes) {
    // NOTE we could try to access Drupal's users with user_load_by_mail($mail) and search for this email -
    //  but we will save that for the activation phase instead.
    $site = UC_GIFT_CARDS_SITE_NAME;
    $dbh = pdoConnect();
    // $owner = $product->data['attributes']; So $attributes = array('Email Address' => 'some@one.com', 'Message' => 'The text of a message.');
    $keys = array_keys($attributes);
    // Figure out which key contains our email address and assign it to $email
    foreach ($keys as $key => $value) {
      if (strpos(strtolower($value), 'email') !== FALSE) {
        $email = $attributes[$value][0];
      }
    }
    /* Is the email address set? If not owner = 0 and return*/
    if ($email == '') {
      $this->owner = json_encode(array('0' => 0));
      return;
    }
    // See if there is an entry in users for this email and site
    $stmt = $dbh->prepare('SELECT * FROM users WHERE email = :email AND site = :site');
    $stmt->execute(array(':email' => $email, ':site' => $site));
    // See if it exists in the DB
    $result = $stmt->fetchAll(PDO::FETCH_ASSOC); 
    if (!empty($result)) {
      $owner = $result[0]['id'];
    } else {
      // Create a new user 
      $stmt = $dbh->prepare('INSERT INTO users (site, uid, email) VALUES (:site, :uid, :email)');
      $stmt->execute(array(
        ':site' => $site,
        ':uid' => NULL,
        ':email' => $email,
          ));  
      $owner = $dbh->lastInsertId();
    }
    $owner_array = $this->getOwner();
    // This fails if it's empty, so create an empty array
    if (gettype($owner_array) != 'array') {
      $owner_array = array();
    }
    array_push($owner_array, $owner);
    $this->owner = json_encode($owner_array);
  }
  public function setOwnerSimple($owner) {
    $this->owner = $owner;
  }
  public function getOwner() {
    return json_decode($this->owner); 
  }
  /**
   * 
   * @return email address for the owner registered for this site, or NULL
   */
  public function getOwnerEmail() {
    $id_array = $this->getOwner();
    $email = array();
    $dbh = pdoConnect();
    foreach ($id_array as $id) {
      $stmt = $dbh->prepare('SELECT * FROM users WHERE id = :id');
      $stmt->execute(array(
        ':id' => $id,
        ));
      $result = $stmt->fetch(PDO::FETCH_ASSOC);
      if ($result) {
        $site = $result['site'];
        $email[$site] = $result['email'];
      }
    }
    if ($email) { 
      return $email;
    } else {
    return FALSE;
    }
  }
  
  /**
   * Updates the user info
   * @param $email
   * @param $uid
   *  Site user id from global $user->uid - may be NULL, either in the param or in the users table
   *   so we can't assume that just because the $uid is passed in, that it already exists in the table
   * @param $site
   *  The site name
   */
  public function updateOwner($uid, $email, $site) {
    $dbh = pdoConnect();
    $owners = $this->getOwner();
    foreach ($owners as $owner) {
      //dpm($owner);
      $stmt = $dbh->prepare('SELECT * FROM users WHERE id = :id');
      $stmt->execute(array(
        ':id' => $owner,
        ));
      $temp = $stmt->fetchAll(PDO::FETCH_ASSOC);
      //dpm($temp);
      $owners_users[] = $temp['0'];
    }
    //dpm($owners_users);
    // See if there is a current entry for this site
    $success = FALSE;
    foreach ($owners_users as $key => $user) {
      //dpm($user);
      if ($user['site'] == $site) {      
        $success = TRUE;
        if ($uid != $user['uid'] || $email != $user['email']) {
          // There is and it's different so update it
          uc_gift_cards_update_user($user['id'], $site, $uid, $email);
        }
      }
    }
    if (!$success) {
      // If the above failed, then something is jacked
      $message = 'Owner info update error on Gift Card ' . $this->getCode();
      watchdog('uc_gift_cards', $message, WATCHDOG_ERROR);      
    }
  }
  
  /**
   * Saves a new Gift Card to the database
   */
  public function saveNew() { 
    // Make our DB connection
    $dbh = pdoConnect();
    // Prepare and execute statement
    $stmt = $dbh->prepare('INSERT INTO gift_cards (code, order_number, purchase_amount, purchase_date, site, balance, purchaser, owner, active) VALUES (:code, :order_number, :purchase_amount, :purchase_date, :site, :balance, :purchaser, :owner, :active)');
    $stmt->execute(array(
      ':code' => $this->getCode(), 
      ':order_number' => $this->getOrderNumber(), 
      ':purchase_amount' => $this->getPurchaseAmount(), 
      ':purchase_date' => $this->getPurchaseDate(), 
      ':site' => $this->getSite(), 
      ':balance' => $this->getBalance(),
      ':purchaser' => $this->getPurchaser(),
      ':owner' => json_encode($this->getOwner()),
      ':active' => $this->getActive(),
        ));
  }
  public function setActive($active) {
    $this->active = $active;
  }
  public function getActive() {
    return $this->active;
  }
  public function displayActiveText() {
    $active = $this->active;
    if ($active === 1) {
      return t('Active');
    }
    return t('Inactive');
  }

} // end of Class
