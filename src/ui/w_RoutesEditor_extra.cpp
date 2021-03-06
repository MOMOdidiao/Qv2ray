// WARNING
// Since it's required for this file to know the content of the macros defined in another CPP file.
// We included an CPP file instead of the proper HPP file.
#include "w_RoutesEditor.cpp"

// Supplementary source file for Routes editor, basically providing routes-related operations.


void RouteEditor::AddInbound(INBOUND in)
{
    QString tag = getTag(in);

    if (inbounds.contains(tag)) {
        tag = tag + "_" + GenerateRandomString(5);
    }

    in["tag"] = tag;
    auto _nodeData = make_unique<QvInboundNodeModel>(make_shared<InboundNodeData>(tag));
    auto &node = nodeScene->createNode(std::move(_nodeData));
    auto pos = QPointF();
    pos.setX(0 + GRAPH_GLOBAL_OFFSET_X);
    pos.setY(inboundNodes.count() * 130 + GRAPH_GLOBAL_OFFSET_Y);
    nodeScene->setNodePosition(node, pos);
    inboundNodes[tag] = &node;
    inbounds[getTag(in)] = in;
}

void RouteEditor::AddOutbound(OUTBOUND out)
{
    QString tag = getTag(out);

    if (outbounds.contains(tag)) {
        tag = tag + "_" + GenerateRandomString(5);
    }

    out["tag"] = tag;
    auto _nodeData = make_unique<QvOutboundNodeModel>(make_shared<OutboundNodeData>(tag));
    auto pos = nodeGraphWidget->pos();
    pos.setX(pos.x() + 850 + GRAPH_GLOBAL_OFFSET_X);
    pos.setY(pos.y() + outboundNodes.count() * 120 + GRAPH_GLOBAL_OFFSET_Y);
    auto &node = nodeScene->createNode(std::move(_nodeData));
    nodeScene->setNodePosition(node, pos);
    outboundNodes[tag] = &node;
    outbounds[tag] = out;
    defaultOutboundCombo->addItem(tag);
}

QString RouteEditor::AddNewRule()
{
    // Add Route
    RuleObject rule;
    //
    rule.QV2RAY_RULE_ENABLED = true;
    rule.QV2RAY_RULE_USE_BALANCER = false;
    // Default balancer tag, it's a random string.
    auto bTag = GenerateRandomString();
    rule.QV2RAY_RULE_TAG = rules.isEmpty() ? tr("Default rule") : (tr("rule") + "-" + GenerateRandomString(6));
    rule.balancerTag = bTag;
    balancers[bTag] = QStringList();
    AddRule(rule);
    return rule.QV2RAY_RULE_TAG;
}

void RouteEditor::AddRule(RuleObject rule)
{
    // Prevent duplicate
    if (ruleNodes.contains(rule.QV2RAY_RULE_TAG)) {
        rule.QV2RAY_RULE_TAG += "-" + GenerateRandomString(5);
    }

    rules[rule.QV2RAY_RULE_TAG] = rule;
    auto pos = nodeGraphWidget->pos();
    pos.setX(pos.x() + 350 + GRAPH_GLOBAL_OFFSET_X);
    pos.setY(pos.y() + ruleNodes.count() * 120 + GRAPH_GLOBAL_OFFSET_Y);
    auto _nodeData = make_unique<QvRuleNodeDataModel>(make_shared<RuleNodeData>(rule.QV2RAY_RULE_TAG));
    auto &node = nodeScene->createNode(std::move(_nodeData));
    nodeScene->setNodePosition(node, pos);

    for (auto inTag : rule.inboundTag) {
        if (!inboundNodes.contains(inTag)) {
            LOG(MODULE_UI, "No inbound tag found for rule: " + rule.QV2RAY_RULE_TAG + ", inbound tag: " + inTag)
            QvMessageBoxWarn(this, tr("No Inbound"), tr("No inbound item found: ") + inTag);
            rule.inboundTag.removeAll(inTag);
        } else {
            auto inboundNode = inboundNodes[inTag];
            nodeScene->createConnection(node, 0, *inboundNode, 0);
        }
    }

    // If not using balancers (use outbound tag)
    if (!rule.QV2RAY_RULE_USE_BALANCER) {
        if (outboundNodes.contains(rule.outboundTag)) {
            DEBUG(MODULE_GRAPH, "Found outbound tag: " + rule.outboundTag + ", for rule: " + rule.QV2RAY_RULE_TAG)
            nodeScene->createConnection(*outboundNodes[rule.outboundTag], 0, node, 0);
        } else {
            LOG(MODULE_GRAPH, "Outbound tag not found: " + rule.outboundTag + ", for: " + rule.QV2RAY_RULE_TAG)
            //QvMessageBoxWarn(this, tr("No outbound tag"), tr("Please connect the rule with an outbound."));
        }
    }

    this->ruleNodes[rule.QV2RAY_RULE_TAG] = &node;
    ruleListWidget->addItem(rule.QV2RAY_RULE_TAG);
}

// Do not use reference here, we need deep
void RouteEditor::RenameItemTag(ROUTE_EDIT_MODE mode, const QString originalTag, QString newTag)
{
    switch (mode) {
        case RENAME_RULE:
            if (rules.contains(originalTag) && ruleNodes.contains(originalTag)) {
                if (rules.contains(newTag) && rules.contains(newTag)) {
                    QvMessageBoxWarn(this, tr("Rename tags"), tr("The new tag has been used, we appended a postfix."));
                    newTag += "_" + GenerateRandomString(5);
                }

                auto node = static_cast<QvRuleNodeDataModel *>(ruleNodes[originalTag]->nodeDataModel());

                if (node == nullptr) {
                    LOG(MODULE_GRAPH, "EMPTY NODE WARN")
                }

                node->setData(newTag);
                //
                rules[newTag] = rules.take(originalTag);
                rules[newTag].QV2RAY_RULE_TAG = newTag;
                ruleNodes[newTag] = ruleNodes.take(originalTag);
                //
                // No other operation needed, but need to rename the one in the ruleOrder list widget.
                auto items = ruleListWidget->findItems(originalTag, Qt::MatchExactly);

                if (items.isEmpty()) {
                    LOG(MODULE_UI, "Cannot find a node: " + originalTag)
                } else {
                    items.first()->setText(newTag);
                }

                if (currentRuleTag == originalTag) {
                    currentRuleTag = newTag;
                }
            } else {
                LOG(MODULE_UI, "There's nothing match " + originalTag + " in the containers.")
            }

            break;

        case RENAME_OUTBOUND:
            if (outbounds.contains(originalTag) && outboundNodes.contains(originalTag)) {
                if (outbounds.contains(newTag) && outboundNodes.contains(newTag)) {
                    QvMessageBoxWarn(this, tr("Rename tags"), tr("The new tag has been used, we appended a postfix."));
                    newTag += "_" + GenerateRandomString(5);
                }

                outbounds[newTag] = outbounds.take(originalTag);
                outboundNodes[newTag] = outboundNodes.take(originalTag);
                auto node = static_cast<QvOutboundNodeModel *>(outboundNodes[newTag]->nodeDataModel());

                if (node == nullptr) {
                    LOG(MODULE_GRAPH, "EMPTY NODE WARN")
                }

                node->setData(newTag);

                // Change outbound tag in rules accordingly.
                for (auto k : rules.keys()) {
                    auto v = rules[k];

                    if (v.outboundTag == originalTag) {
                        v.outboundTag = newTag;
                        // Put this inside the if block since no need an extra operation if the condition is false.
                        rules[k] = v;
                    }
                }
            } else {
                LOG(MODULE_UI, "Failed to rename an outbound --> Item not found.")
            }

            break;

        case RENAME_INBOUND:
            if (inbounds.contains(originalTag) && inboundNodes.contains(originalTag)) {
                if (inbounds.contains(newTag) && inboundNodes.contains(newTag)) {
                    QvMessageBoxWarn(this, tr("Rename tags"), tr("The new tag has been used, we appended a postfix."));
                    newTag += "_" + GenerateRandomString(5);
                }

                inbounds[newTag] = inbounds.take(originalTag);
                inboundNodes[newTag] = inboundNodes.take(originalTag);
                auto node = static_cast<QvInboundNodeModel *>(inboundNodes[newTag]->nodeDataModel());

                if (node == nullptr) {
                    LOG(MODULE_GRAPH, "EMPTY NODE WARN")
                }

                node->setData(newTag);

                // Change inbound tag in rules accordingly.
                // k -> rule tag
                // v -> rule object
                for (auto k : rules.keys()) {
                    auto v = rules[k];

                    if (v.inboundTag.contains(originalTag)) {
                        v.inboundTag.append(newTag);
                        v.inboundTag.removeAll(originalTag);
                        // Put this inside the if block since no need an extra operation if the condition is false.
                        rules[k] = v;
                    }
                }
            } else {
                LOG(MODULE_UI, "Failed to rename an outbound --> Item not found.")
            }

            break;
    }
}
