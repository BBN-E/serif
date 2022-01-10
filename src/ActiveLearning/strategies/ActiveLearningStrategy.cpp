#include "Generic/common/leak_detection.h"
#include "ActiveLearning/ActiveLearningData.h"
#include "ActiveLearningStrategy.h"

ActiveLearningStrategy::ActiveLearningStrategy(ActiveLearningData_ptr al_data) 
: _alData(al_data)
{
}

void ActiveLearningStrategy::modelUpdate() {
}
