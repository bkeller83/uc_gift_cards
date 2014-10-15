<?php

/**
 * @file General Configuration settings for UC Gift Cards
 *  These settings are configured here because once set, they should not be changed.
 * For example, if you change the site name variable after gift cards have been created,
 * those generated cards will no longer work.
 */

/*
 * Site Name
 *  This is used to associate gift card codes and users with web sites and is
 * integral to the multi site feature of this module. Once it is set for a site,
 * it should not be changed.
 */ 
define('UC_GIFT_CARDS_SITE_NAME', 'wallquotes dev');

/**
 * Gift Card Code Length 
 *  In case we want to change this for some reason - note 
 * that form inputs are tied to this as well so changing it after codes were generated
 * would make the old ones inaccessible.
 */
define('UC_GIFT_CARDS_CODE_LENGTH', 6);

/**
 * Defines the Node ID for our Gift Card Product
 *  This will need to be defined for a site after the Gift Card product is created.
 *  299 for wallquotes dev; 514 for workfordirt dev; 299 wallquotes.com
 */
define('UC_GIFT_CARDS_NODE_ID', '299');