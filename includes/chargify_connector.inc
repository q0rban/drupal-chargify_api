<?php
// $Id$

/**
 * @file
 *   Chargify API Connector class.
 *   Adapted from Christopher Lewis' Chargify API wrapper for PHP, found at 
 *   http://github.com/lewsid/chargify_api_php
 */
class chargify_connector {
  private $api_key      = variable_get('chargify_api_api_key', '');
  private $test_api_key = variable_get('chargify_api_test_api_key', '');
  private $domain       = variable_get('chargify_api_domain', '');
  private $test_domain  = variable_get('chargify_api_test_domain', '');
  
  private $active_api_key;
  private $active_domain;
  
  public function __construct() {
    if (variable_get('chargify_api_test_mode', TRUE)) {
      $this->active_api_key = $this->test_api_key;
      $this->active_domain  = $this->test_domain;
    }
    else {
      $this->active_api_key = $this->api_key;
      $this->active_domain  = $this->domain;
    }
  }

  public function customers_get_xml($page_num = 1) {
    $path = "customers.xml?page=$page_num";
    $cmd = $this->connector_command($path);

    exec($cmd, $output);
    $xml = implode("\n", $output);

    return $xml;
  }

  public function customer_get_xml($id) {
    $path = "customers/$id.xml";
    $cmd = $this->connector_command($path);

    exec($cmd, $output);
    $xml = implode("\n", $output);

    return $xml;
  }

  public function customer_get_subscriptions_xml($id) {
    $path = "customers/$id/subscriptions.xml";
    $cmd = $this->connector_command($path);

    exec($cmd, $output);
    $xml = implode("\n", $output);

    return $xml;
  }

  public function products_get_xml() {
    $path = "products.xml";
    $cmd = $this->connector_command($path);

    exec($cmd, $output);
    $xml = implode("\n", $output);

    return $xml;
  }

  /**
   * Example XML:     
   *
   * <?xml version="1.0" encoding="UTF-8"?>
   * <subscription>
   *   <product_handle>' . $product_id . '</product_handle>
   *   <customer_attributes>
   *     <first_name>first</first_name>
   *     <last_name>last</last_name>
   *     <email>email@email.com</email>
   *   </customer_attributes>
   *   <credit_card_attributes>
   *     <first_name>first</first_name>
   *     <last_name>last</last_name>
   *     <billing_address>1 Infinite Loop</billing_address>
   *     <billing_city>Somewhere</billing_city>
   *     <billing_state>CA</billing_state>
   *     <billing_zip>12345</billing_zip>
   *     <billing_country>USA</billing_country>
   *     <full_number>41111111111111111</full_number>
   *     <expiration_month>11</expiration_month>
   *     <expiration_year>2012</expiration_year>
   *   </credit_card_attributes>
   * </subscription>
   */

  public function createCustomerAndSubscription($post_xml) {
    exec('curl -u ' . $this->active_api_key . ':x -H Content-Type:application/xml https://' . $this->active_domain 
      . '.chargify.com/subscriptions.xml --data-binary "' . $post_xml . '"', $output);

    $xml = implode("\n", $output);
    $tree = new SimpleXMLElement($xml);

    if (isset($tree->error)) { 
      return $tree;
    }

    return new ChargifySubscription($tree);
  }

  public function customers_get() {
    $xml = $this->customers_get_xml();
    $customers = new SimpleXMLElement($xml);
    $customer_objects = array();

    foreach ($customers as $customer) {
      $customer_objects[] = new ChargifyCustomer($customer);
    }

    return $customer_objects;
  }
  
  public function customer_get($id) {
    $xml = $this->customer_get_xml($id);
    $customer_xml_node = new SimpleXMLElement($xml);

    return new ChargifyCustomer($customer_xml_node);
  }
  
  public function customer_get_subscriptions($id) {
    $xml = $this->customer_get_subscriptions_xml($id);
    $subscriptions = new SimpleXMLElement($xml);
    $subscription_objects = array();

    foreach ($subscriptions as $subscription) {
      $subscription_objects[] = new ChargifySubscription($subscription);
    }

    return $subscription_objects;
  }
  
  public function get_products() {
    $xml = $this->products_get_xml();
    $all_products = new SimpleXMLElement($xml);
    $product_objects = array();
    
    foreach ($all_products as $product) {
      $product_objects[] = new ChargifyProduct($product);
    }

    return $product_objects;
  }

  /**
   * Constructs the string command that connects to Chargify.
   *
   * @param $path
   *   The path and queries, appended to the base Chargify url.
   * @return
   *   String, command to connect to Chargify.
   */
  function connector_command($path = '') {
    return "curl -u {$this->active_api_key}:x https://{$this->active_domain}.chargify.com/" . $path;
  }
}

?>