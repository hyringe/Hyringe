package hycallparser.parameter;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

public class ParameterList extends Parameter {

    private List<Parameter> values;

    public ParameterList(List<Parameter> values) {
        this.values = new ArrayList<>(values);
    }

    @Override
    public boolean isList() {
        return true;
    }

    @Override
    public List<Parameter> getValues() {
        return values;
    }

    @Override
    public String toString() {
        return "[" + values.stream().map(Object::toString).collect(Collectors.joining(", ")) + "]";
    }

    public Optional<Parameter> findKVPwithKey(String key) {
        return values.stream().filter(p -> p.isKeyValue()).filter(p -> p.getKey().equals(key)).findFirst();
    }
}
