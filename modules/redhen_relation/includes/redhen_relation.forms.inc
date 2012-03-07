<?php

/**
 * @file
 * Form definition and handling for redhen relations.
 */

/**
 * Return a form array for adding/editing a connection.
 *
 * @param $form
 * @param $form_state
 * @param RedhenContact|RedhenOrg $entity
 * @param stdClass $relation
 * @return array
 */
function redhen_relation_connection_form($form, &$form_state, $entity) {
  $relation_types = relation_get_available_types($entity->entityType(), $entity->bundle());

  // instantiate new relation based on default or submitted relation type
  $relation_type = isset($form_state['values']['relation_type']) ?
    $form_state['values']['relation_type'] :
    reset($relation_types)->relation_type;
  $relation = relation_create($relation_type, array());

  // store contact and relation entities for use on submit
  $form_state['entity'] = $entity;
  $form_state['relation'] = $relation;

  // load all available relation types
  $options = array();
  foreach($relation_types as $type) {
    $options[$type->relation_type] = $type->label;
  }
  $form['relation_type'] = array(
    '#title' => t('Connection type'),
    '#type' => 'select',
    '#options' => $options,
    '#default value' => isset($relation) ? $relation->relation_type : NULL,
    '#ajax' => array(
      'callback' => 'redhen_relation_relation_type_callback',
      'wrapper' => 'redhen_relation_fields',
      'method' => 'replace',
      'effect' => 'fade',
      'progress' => array(
        'type' => 'throbber',
        'message' => t('Retrieving fields for this connection type.')
      )
    )
  );

  // determine the entity type we're going to relate to
  $entity_type_to_relate = 'redhen_org';
  if ($entity->entityType() === 'redhen_org' || $relation->relation_type == 'redhen_connection') {
    $entity_type_to_relate = 'redhen_contact';
  }
  $form_state['entity_to_relate_type'] = $entity_type_to_relate;

  $form['fields'] = array(
    '#type' => 'fieldset',
    '#id' => 'redhen_relation_fields'
  );

  $available_entities = redhen_relation_get_available_entities($entity, $entity_type_to_relate, $relation->relation_type);

  if (!empty($available_entities)) {
    $options = array('' => t('-- Select --'));
    foreach ($available_entities as $available_entity) {
      $entity_id = entity_id($available_entity->entityType(), $available_entity);
      $options[$entity_id] = entity_label($available_entity->entityType(), $available_entity);
    }

    $info = entity_get_info($entity_type_to_relate);
    $plural_label = isset($info['plural label']) ? $info['plural label'] : $info['label'] . 's';
    $form['fields']['entity_to_relate'] = array(
      '#title' => $plural_label,
      '#type' => 'select',
      '#options' => $options,
      '#required' => TRUE
    );

    // attach any fields
    field_attach_form('relation', $relation, $form['fields'], $form_state);

    // hide the endpoints field widget. @TODO: Find out why appearing.
    $form['fields']['endpoints']['#access'] = FALSE;
  }
  else {
    $form['fields']['no_connections'] = array(
      '#markup' => t('Sorry, there are no available %type connections.', array('%type' => $relation->relation_type))
    );
  }

  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save connection'),
    '#weight' => 999,
    '#access' => !empty($available_entities)
  );

  return $form;
}

/**
 * AJAX callback to return fields for a given relation type.
 */
function redhen_relation_relation_type_callback($form, $form_state) {
  return $form['fields'];
}

/**
 * Validation handler for redhen_relation_connection_form().
 */
function redhen_relation_connection_form_validate($form, &$form_state) {
  // only validate if a entity to relate has been selected
  if (isset($form_state['values']['entity_to_relate'])) {
    $entity = $form_state['entity'];
    $relation = $form_state['relation'];

    // set the endpoints field to the selected values.
    $endpoints =array (
      array('entity_type' => $entity->entityType(),
        'entity_id' => $entity->internalIdentifier()),
      array('entity_type' => $form_state['entity_to_relate_type'],
        'entity_id' => $form_state['values']['entity_to_relate']),
    );
    $form_state['values']['endpoints'][LANGUAGE_NONE] = $endpoints;

    // set the relation type based on selected value before validating
    $relation->relation_type = $form_state['values']['relation_type'];

    field_attach_form_validate('relation', $relation, $form, &$form_state);
  }
}

/**
 * Submit handler for redhen_relation_connection_form().
 */
function redhen_relation_connection_form_submit($form, &$form_state) {
  $entity = $form_state['entity'];
  $relation = $form_state['relation'];

  $relation->relation_type = $form_state['values']['relation_type'];

  field_attach_submit('relation', $relation, $form, $form_state);
  if (relation_save($relation)) {
    drupal_set_message(t('The connection has been saved.'));
    $uri = entity_uri($entity->entityType(), $entity);
    $form_state['redirect'] = $uri['path'] . "/connections";
  }
  else {
    drupal_set_message(t('The connection could not be saved.'), 'error');
  }
}