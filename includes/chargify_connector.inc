<?php
// $Id: chargify_connector.inc,v 1.2 2010/03/09 17:31:38 q0rban Exp $

/**
 * @file
 *   Chargify API Connector class.
 *   Adapted from Christopher Lewis' Chargify API wrapper for PHP, found at 
 *   http://github.com/lewsid/chargify_api_php
 */

/**
 * The Chargify connector class.  Yay!
 */
class chargify_connector {
  private $active_api_key;
  private $active_domain;

  public function __construct() {
    if (variable_get('chargify_api_test_mode', TRUE)) {
      $this->active_api_key = variable_get('chargify_api_test_api_key', '');
      $this->active_domain  = variable_get('chargify_api_test_domain', '');
    }
    else {
      $this->active_api_key = variable_get('chargify_api_api_key', '');
      $this->active_domain  = variable_get('chargify_api_domain', '');
    }
  }

  /**
   * Creates a Chargify subscription.
   *
   * @param $data
   *   String, xml or json post data to be posted to chargify.
   *
   *   Example XML:     
   *   
   *   <?xml version="1.0" encoding="UTF-8"?>
   *   <subscription>
   *     <product_handle>' . $product_id . '</product_handle>
   *     <customer_attributes>
   *       <first_name>first</first_name>
   *       <last_name>last</last_name>
   *       <email>email@email.com</email>
   *     </customer_attributes>
   *     <credit_card_attributes>
   *       <first_name>first</first_name>
   *       <last_name>last</last_name>
   *       <billing_address>1 Infinite Loop</billing_address>
   *       <billing_city>Somewhere</billing_city>
   *       <billing_state>CA</billing_state>
   *       <billing_zip>12345</billing_zip>
   *       <billing_country>USA</billing_country>
   *       <full_number>41111111111111111</full_number>
   *       <expiration_month>11</expiration_month>
   *       <expiration_year>2012</expiration_year>
   *     </credit_card_attributes>
   *   </subscription>
   *
   * @param $data_type
   *   The type of data that is being posted, 'json' or 'xml'.
   * @return
   *   A chargify_subscription object.
   */
  public function subscription_create($data, $data_type = 'xml') {
    $options = array(
      CURLOPT_HTTPHEADER => array("Content-Type:application/$data_type"),
      CURLOPT_BINARYTRANSFER => TRUE,
      CURLOPT_POSTFIELDS => $data,
    );

    $ch = $this->curl_handle('subscriptions.xml', $options);

    if ($xml = $this->connect($ch)) {
      $tree = new SimpleXMLElement($xml);

      if (isset($tree->error)) { 
        return $tree;
      }

      return new chargify_subscription($tree);
    }
    return FALSE;
  }

  /**
   * Returns a Chargify subscription by ID.
   *
   * @param $id
   *   The id or reference.
   * @param $type
   *   Either 'id' or 'ref' depending on $id.
   * @return
   *   A chargify_subscription object.
   */
  public function subscription_get($id) {
    if ($xml = $this->subscription_get_xml($id)) {
      $subscription_xml_node = new SimpleXMLElement($xml);

      return new chargify_subscription($subscription_xml_node);
    }
  }

  /**
   * Deletes a subscription
   *
   * @param $id
   *   The Chargify subscription id.
   * @param $data
   *   String, xml or json post data to be posted to chargify.
   *
   *   Example XML:
   *   <?xml version="1.0" encoding="UTF-8"?>
   *   <subscription>
   *     <cancellation_message>
   *       Canceling the subscription via the API
   *     </cancellation_message>
   *   </subscription>
   *
   * @param $data_type
   *   The type of data that is being posted, 'json' or 'xml'.
   * @return
   *   Boolean value on success or failure.
   */
  public function subscription_delete($id, $data, $data_type = 'xml') {
    $options = array(
      CURLOPT_HTTPHEADER => array("Content-Type:application/$data_type"),
      CURLOPT_BINARYTRANSFER => TRUE,
      CURLOPT_POSTFIELDS => $data,
      CURLOPT_CUSTOMREQUEST => 'DELETE',
    );
    $ch = $this->curl_handle("subscriptions/$id.xml", $options);

    return $this->connect($ch);
  }

  /**
   * Returns all customers.
   *
   * @return
   *   An array of all Chargify customers, each item being a chargify_customer 
   *   object.
   */
  public function customers_get() {
    $xml = $this->customers_get_xml();
    $customers = new SimpleXMLElement($xml);
    $customer_objects = array();

    foreach ($customers as $customer) {
      $customer_objects[] = new chargify_customer($customer);
    }

    return $customer_objects;
  }

  /**
   * Returns a Chargify customer, by ID or handle.
   *
   * @param $value
   *   The id or reference.
   * @param $type
   *   Either 'id' or 'ref' depending on $id.
   * @return
   *   A chargify_customer object.
   */
  public function customer_get($value, $type = 'id') {
    switch ($type) {
      case 'ref':
        $xml = $this->customer_get_xml_by_ref($value);
        break;
      default:
        $xml = $this->customer_get_xml($value);
    }
    $customer_xml_node = new SimpleXMLElement($xml);

    return new chargify_customer($customer_xml_node);
  }

  /**
   * Returns all subscriptions for a customer
   *
   * @param $id
   *   The chargify customer ID.
   * @return
   *   An array of all Chargify subscriptions for a single customer, each item 
   *   being a chargify_subscription object.
   */
  public function customer_get_subscriptions($id) {
    if ($xml = $this->customer_get_subscriptions_xml($id)) {
      $subscriptions = new SimpleXMLElement($xml);
      $subscription_objects = array();
  
      foreach ($subscriptions as $subscription) {
        $subscription_objects[] = new chargify_subscription($subscription);
      }
  
      return $subscription_objects;
    }
  }

  /**
   * Returns all products.
   *
   * @return
   *   An array of all Chargify products, each item being a chargify_product object.
   */
  public function products_get() {
    if ($xml = $this->products_get_xml()) {
      $all_products = new SimpleXMLElement($xml);
      $product_objects = array();

      foreach ($all_products as $product) {
        $product_objects[] = new chargify_product($product);
      }

      return $product_objects;
    }
  }

  /**
   * Returns a chargify product, by ID or handle.
   *
   * @param $value
   *   The numeric id or string handle.
   * @param $type
   *   Either 'id' or 'handle' depending on $value.
   * @return
   *   A chargify_product object.
   */
  public function product_get($value, $type = 'id') {
    if ($xml = $this->product_get_xml($value, $type)) {
      $xml_obj = new SimpleXMLElement($xml);
  
      return new chargify_product($xml_obj);
    }
  }

  public function subscription_get_xml($id) {
    $path = "subscriptions/$id.xml";
    $ch = $this->curl_handle($path);

    return $this->connect($ch);
  }

  public function customers_get_xml($page_num = 1) {
    $path = "customers.xml?page=$page_num";
    $ch = $this->curl_handle($path);

    return $this->connect($ch);
  }

  public function customer_get_xml($id) {
    $path = "customers/$id.xml";
    $ch = $this->curl_handle($path);

    return $this->connect($ch);
  }

  public function customer_get_xml_by_ref($ref) {
    $ref = urlencode($ref);
    $path = "customers/lookup.xml?reference=$ref";
    $ch = $this->curl_handle($path);

    return $this->connect($ch);
  }

  public function customer_get_subscriptions_xml($id) {
    $path = "customers/$id/subscriptions.xml";
    $ch = $this->curl_handle($path);

    return $this->connect($ch);
  }

  public function products_get_xml() {
    $path = "products.xml";
    $ch = $this->curl_handle($path);

    return $this->connect($ch);
  }

  public function product_get_xml($value, $type = 'id') {
    $path = 'products/';
    $path .= ($type == 'handle') ? "handle/$value.xml" : "$value.xml";

    $ch = $this->curl_handle($path);

    return $this->connect($ch);
  }

  /**
   * Connect to Chargify by executing the cURL handle.
   *
   * @param $ch
   *   A cURL handle returned by curl_init().
   * @return
   *   The result on success, FALSE on failure.
   */
  private function connect(&$ch) {
    $result = curl_exec($ch);
    curl_close($ch);
    return trim($result);
  }

  /**
   * Constructs the cURL handle that connects to Chargify.
   *
   * @param $path
   *   The path and queries, appended to the base Chargify url.
   * @param $options
   *   An array specifying which options to set and their values. The keys should 
   *   be valid curl_setopt() constants or their integer equivalents. 
   * @return
   *   A cURL handle.
   */
  private function curl_handle($path = '', $options = array()) {
    // Initialize the curl handle.
    $ch = curl_init();

    $options += array(
      CURLOPT_URL => "https://{$this->active_domain}.chargify.com/$path",
      CURLOPT_SSL_VERIFYPEER => FALSE,
      CURLOPT_USERPWD => "{$this->active_api_key}:x",
      CURLOPT_RETURNTRANSFER => TRUE,
      //CURLOPT_VERBOSE => TRUE,
    );

    curl_setopt_array($ch, $options);

    return $ch;
  }
}