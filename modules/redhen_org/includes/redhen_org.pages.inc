<?php

/**
 * @file
 * Page callbacks for redhen organizations.
 */

module_load_include('inc', 'redhen', 'includes/redhen.forms');

/**
 * Page callback for org overview page.
 */
function redhen_org_page() {

  // setup the header for both the query and table
  $header = array(
    'label' => array('field' => 'label', 'type' => 'property', 'data' => 'Name', 'sort' => 'asc', 'specifier' => 'label'),
    'type' => array('field' => 'type', 'type' => 'property', 'data' => 'Type', 'sort' => 'asc', 'specifier' => 'type'),
    'updated' => array('field' => 'updated', 'type' => 'property', 'data' => 'Updated', 'sort' => 'desc', 'specifier' => 'updated'),
    'state' => array('field' => 'state', 'type' => 'property', 'data' => 'State', 'sort' => 'desc', 'specifier' => 'redhen_state'),
    'Actions',
  );

  // need to ensure the query doesn't execute when posing the form
  $result = FALSE;
  if (!isset($_POST['form_id'])) {
    $bundle = isset($_GET['bundle']) ? $_GET['bundle'] : '';
    $properties = isset($_GET['properties']) ? $_GET['properties'] : array();
    $fields = isset($_GET['fields']) ? $_GET['fields'] : array();
    $result = redhen_filter_query('redhen_org', $header, $bundle, $properties, $fields);
  }

  $render = array(
    'form' => drupal_get_form('redhen_filter_form', 'redhen_org'),
    );

  $rows = array();
  if ($result) {
    $orgs = redhen_org_load_multiple(array_keys($result['redhen_org']));

    foreach($orgs as $org) {
      $actions = array(
        l('view', 'redhen/org/' . $org->org_id),
        l('edit', 'redhen/org/' . $org->org_id . '/view/edit', array('query' => drupal_get_destination())),
        l('archive', 'redhen/org/' . $org->org_id . '/archive', array('query' => drupal_get_destination())),
        l('delete', 'redhen/org/' . $org->org_id . '/view/delete', array('query' => drupal_get_destination())),
      );

      $redhen_org_type = redhen_org_type_load($org->type);
      $rows[] = array(
        'data' => array(
          $org->label,
          $redhen_org_type->label,
          format_date($org->updated),
          $org->redhen_state,
          implode(' | ', $actions))
      );
    }
    $render['table'] = array(
        '#theme' => 'table',
        '#header' => $header,
        '#rows' => $rows
      );
    $render['pager'] = array(
        '#theme' => 'pager'
      );
  }
  else {
    // no results, set a message
    $render['no-result'] = array(
      '#type' => 'markup',
      '#markup' => t('Sorry, there are no organizations that match your criteria.'),
    );
  }

  return $render;
}

/**
 * Page callback for adding an organization.
 *
 * @param string $type
 *   The type of organization to return a form for.
 */
function redhen_org_types_list_page($type = 'default') {
  $item = menu_get_item();
  $content = system_admin_menu_block($item);

  // Bypass the redhen_org/add listing if only one contact type is available.
  if (count($content) == 1) {
    $item = array_shift($content);
    drupal_goto($item['href']);
  }

  $render = array(
    '#theme' => 'redhen_org_add_list',
    '#content' => $content,
  );

  return $render;
}

/**
 * Page callback for adding an organization.
 *
 * @param $type
 * @return array|mixed
 */
function redhen_org_add_page($type) {
  $org = entity_get_controller('redhen_org')->create(array('type' => $type));
  module_load_include('inc', 'redhen_org', 'includes/redhen_org.forms');
  return drupal_get_form('redhen_org_org_form', $org);
}

/**
 * Autocomplete helper
 *
 * @param $string
 *  String for search
 */
function _redhen_org_autocomplete($string) {
  $matches = array();

  // build the query
  $query = new EntityFieldQuery();

  $query
    ->entityCondition('entity_type', 'redhen_org', '=')
    ->tablesort($header)
    ->pager(10);

  $query->propertyCondition('label', '%' . db_like($string) .'%', 'LIKE');

  $result =  $query->execute();

  $orgs = redhen_org_load_multiple(array_keys($result['redhen_org']));

  // save the query to matches
  foreach ($orgs as $row) {
    $key = "$row->label ($row->org_id)";
    // Names containing commas or quotes must be wrapped in quotes.
    if (strpos($key, ',') !== FALSE || strpos($key, '"') !== FALSE) {
      $key = '"' . str_replace('"', '""', $key) . '"';
    }
    $matches[$key] = '<div class="redhen-autocomplete">' . check_plain($row->label) . '</div>';
  }

  // Return the result to the form in json
  drupal_json_output($matches);
}