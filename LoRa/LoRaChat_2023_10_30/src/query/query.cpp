#include "query.h"

static const char* QUERY_TAG = "QueryService";

void Query::init() {
    pinMode(LED, OUTPUT);
}

String Query::queryOn() {
    digitalWrite(LED, LED_ON);
    ESP_LOGV(QUERY_TAG, "Led On");
    //ESP_LOGV(LED_TAG, "Led On to %X", dst);
    //Log.verboseln(F("Query On"));
    return "Query On";
}

String Query::queryOn(uint16_t dst) {
    ESP_LOGV(QUERY_TAG, "Led On to %X", dst);
    if (dst == LoraMesher::getInstance().getLocalAddress())
        return queryOn();

    DataMessage* msg = getQueryMessage(QueryCommand::services, dst);
    MessageManager::getInstance().sendMessage(messagePort::LoRaMeshPort, msg);

    delete msg;

    return "Query On";
}

String Query::queryOff() {
    digitalWrite(LED, LED_OFF);
    //Log.verboseln(F("Query Off"));
    ESP_LOGV(QUERY_TAG, "Led Off");
    return "Query Off";
}

String Query::queryOff(uint16_t dst) {
    ESP_LOGV(QUERY_TAG, "Led Off to %X", dst);
    if (dst == LoraMesher::getInstance().getLocalAddress())
        return queryOff();

    DataMessage* msg = getQueryMessage(QueryCommand::routes, dst);
    MessageManager::getInstance().sendMessage(messagePort::LoRaMeshPort, msg);

    delete msg;

    return "Query Off";
}

String Query::getJSON(DataMessage* message) {
    Log.verboseln(F("FF: Query::getJSON"));
    QueryMessage* queryMessage = (QueryMessage*) message;

    StaticJsonDocument<400> doc;  // FF: was 200
    //DynamicJsonDocument doc(400); // FF: do not make dynamic

    JsonObject data = doc.createNestedObject("data");

    queryMessage->serialize(data);

    String json;
    serializeJson(doc, json);

    return json;
}

DataMessage* Query::getDataMessage(JsonObject data) {
    QueryMessage* queryMessage = new QueryMessage();
    Log.infoln(F("Query::getDataMessage new QueryMessage()") );
    //Log.infoln(F("FF: Heap after new QueryMessage() getFreeHeap() :%d"), ESP.getFreeHeap() );
    Log.infoln(F("FF: Heap after new QueryMessage(): %d %d %d %d"),ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());

    queryMessage->deserialize(data);  //FF commented.

    queryMessage->messageSize = sizeof(QueryMessage) - sizeof(DataMessageGeneric);

    return ((DataMessage*) queryMessage);
}

DataMessage* Query::getQueryMessage(QueryCommand command, uint16_t dst) {
    QueryMessage* queryMessage = new QueryMessage();
    Log.infoln(F("FF Query::getQueryMessage: new QueryMessage()") );

    queryMessage->messageSize = sizeof(QueryMessage) - sizeof(DataMessageGeneric);

    queryMessage->queryCommand = command;

    queryMessage->appPortSrc = appPort::QueryApp;
    queryMessage->appPortDst = appPort::QueryApp;

    queryMessage->addrSrc = LoraMesher::getInstance().getLocalAddress();
    Log.infoln(F("Query::getQueryMessage: queryMessage->addrSrc: %d"), queryMessage->addrSrc );
    queryMessage->addrDst = dst;

    return (DataMessage*) queryMessage;
}

void Query::processReceivedMessage(messagePort port, DataMessage* message) {
    QueryMessage* queryMessage = (QueryMessage*) message;

    Log.infoln(F("FF: Query::processReceivedMessage  perform local actions") );

    queryCommandS = queryMessage->queryCommand;   //FF added
    queryvalueS = queryMessage->queryValue;    //FF added
    
    Log.verboseln(F("FF in Query::processReceivedMessage queryCommandS %d"),queryCommandS );
    Log.verboseln(F("FF in Query::processReceivedMessage queryvalueS %d"),queryvalueS);

    switch (queryMessage->queryCommand) {
        case QueryCommand::services:
            queryOn();
            break;
        case QueryCommand::routes:
            queryOff();
            break;
        default:
            break;
    }
    
    //delete message;  // FF tried, makes segmentation fault. UPATDE: delete message is in MQTT: void MqttService::processReceivedMessageFromMQTT(String& topic, String& payload) { 
    //delete queryMessage; // FF tried, 

    //Query& query = Query::getInstance();
    //query.
    Log.infoln(F("FF: Query::processReceivedMessage   createAndSendQuery()") );
    createAndSendQuery();
}

void Query::createAndSendQuery() {
    // uint8_t metadataSize = 1; //TODO: This should be dynamic with an array of sensors
    // uint16_t metadataSensorSize = metadataSize * sizeof(MetadataSensorMessage);
    uint16_t messageWithHeaderSize = sizeof(QueryMessage);// + metadataSensorSize;

    //QueryMessage* message = (QueryMessage*) malloc(messageWithHeaderSize);
    QueryMessage* message = new QueryMessage();  // FF: instead of malloc

    Log.verboseln(F("FF Query::createAndSendQuery: Sending query message"));

    if (message) {

        message->appPortDst = appPort::MQTTApp;
        message->appPortSrc = appPort::QueryApp;
        message->addrSrc = LoraMesher::getInstance().getLocalAddress();
        //Log.verboseln(F("Query::createAndSendQuery message->addrSrc %d"),message->addrSrc);
        message->addrDst = 0;
        message->messageId = queryId++;

        message->queryCommand = queryCommandS;   //FF added
        message->queryValue = queryvalueS;    //FF added


        message->messageSize = messageWithHeaderSize - sizeof(DataMessageGeneric);
        //message->gps = GPSService::getInstance().getGPSMessage();
        //message->flSendTimeInterval = FL_UPDATE_DELAY;

        // message->metadataSize = metadataSize;

        //TODO: This should be a vector of sensors
        // Temperature& temperature = Temperature::getInstance();

        // MetadataSensorMessage* tempMetadata = temperature.getMetadataMessage();
        // memcpy(message->sensorMetadata, tempMetadata, sizeof(MetadataSensorMessage));

        // signData(message);

        MessageManager::getInstance().sendMessage(messagePort::MqttPort, (DataMessage*) message);

        //free(message);
        delete message;
    }
    else {
        Log.errorln(F("Failed to allocate memory for query message"));
    }
}
