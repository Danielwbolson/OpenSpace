import React from 'react';
import Property from './Property';
import NumericInput from '../../common/Input/NumericInput/NumericInput';
import Row from '../../common/Row/Row';

class VecProperty extends Property {
  static jsonToLua(json) {
    return json.replace('[', '').replace(']', '');
  }

  onChange(index) {
    return (event) => {
      const stateValue = JSON.parse(this.state.value);
      const { value } = event.currentTarget;
      stateValue[index] = parseFloat(value);

      this.saveValue(VecProperty.jsonToLua(JSON.stringify(stateValue)));

      // optimistic UI change!
      this.setState({ value: JSON.stringify(stateValue) });
    };
  }

  render() {
    const { Description } = this.props;
    const { SteppingValue, MaximumValue, MinimumValue } = Description.AdditionalData;
    const firstLabel = (<span>
      { Description.Name } { this.descriptionPopup }
    </span>);

    // eslint-disable-next-line react/no-array-index-key
    const values = JSON.parse(this.state.value)
      .map((value, index) => ({ key: `${Description.Name}-${index}`, value }));

    return (
      <Row>
        { values.map((component, index) => (
          <NumericInput
            key={component.key}
            value={component.value}
            placeholder={index === 0 ? firstLabel : ' '}
            onChange={this.onChange(index)}
            step={SteppingValue[index]}
            max={MaximumValue[index]}
            min={MinimumValue[index]}
          />
        ))}
      </Row>
    );
  }
}

export default VecProperty;
