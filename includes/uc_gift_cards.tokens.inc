<?php
/**
 * @file
 * Provides token support for uc_gift_cards
 */

/** 
 * Implements hook_token_info().
 */
function uc_gift_cards_token_info() {
  $types = array(
    'uc_gift_cards' => array(
      'name' => t('Gift Cards'),
      'description' => t('Tokens for Ubercart module UC Gift Cards'),
      'needs-data' => 'uc_gift_cards',
    ),
  );
  
  $tokens = array(
    'uc_gift_cards' => array(
      'code' => array(
        'name' => t('Code'),
        'description' => t('The Gift Card Code.'),
      ),
      'value' => array(
        'name' => t('Value'),
        'description' => t('The Value of the Gift Card.'),
      ),
      'from' => array(
        'name' => t('From'),
        'description' => t('The email address of the customer that purchased the Gift Card.'),
      ),
      'activate' => array(
        'name' => t('Activate'),
        'description' => t('Displays the text \'Activate\' as a link to the full URL gift-cards/activate page'),
      ),
    ),
  );
  
  return array('types' => $types, 'tokens' => $tokens);
}

/**
 * Implements hook_tokens().
 *  The gift card object needs to be passed in the data array
 * $data = array('gift_card' => $gift_card,)
 */
function uc_gift_cards_tokens($type, $tokens, $data = array(), $options = array()) {
  $replacements = array();
  
  if ($type == 'uc_gift_cards') {
    $gift_card = $data['gift_card'];
    foreach ($tokens as $name => $original) {
      switch($name) {
        case 'code':
          $replacements[$original] = $gift_card->getCode();
          break;
        case 'value':
          $replacements[$original] = number_format($gift_card->getPurchaseAmount(), 2);
          break;
        case 'from':
          $replacements[$original] = $gift_card->getPurchaserEmail();
          break;  
        case 'activate':
          global $base_url;
          $replacements[$original] = '<a href="' . $base_url . '/gift-cards/activate">Activate</a>';
          break;
      }
    }
  }
  
  return $replacements;
}